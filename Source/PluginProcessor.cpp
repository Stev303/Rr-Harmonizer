#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr float pi = 3.14159265358979323846f;
juce::StringArray noteNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

RrHarmonizerAudioProcessor::RrHarmonizerAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

RrHarmonizerAudioProcessor::APVTS::ParameterLayout RrHarmonizerAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout p;
    auto choice = [] (const juce::String& id, const juce::String& name, juce::StringArray options, int def)
    { return std::make_unique<juce::AudioParameterChoice> (id, name, options, def); };
    auto f = [] (const juce::String& id, const juce::String& name, float lo, float hi, float def, float step = 0.0f)
    {
        juce::NormalisableRange<float> r (lo, hi, step);
        return std::make_unique<juce::AudioParameterFloat> (id, name, r, def);
    };
    p.add (choice ("mode", "Harmony mode", { "Scale", "Chord", "Fixed" }, 0));
    p.add (choice ("key", "Key", noteNames, 1));
    p.add (choice ("scale", "Scale", { "Major", "Natural Minor", "Harmonic Minor", "Dorian", "Mixolydian", "Chromatic" }, 0));
    p.add (choice ("source", "Chord source", { "Music", "Manual" }, 0));
    p.add (choice ("voices", "Voice count", { "1", "2", "3", "4" }, 2));
    p.add (choice ("latency", "Pitch range", { "Low", "Mid", "High" }, 1));
    p.add (f ("gate", "Gate", -80.0f, -10.0f, -50.0f, 1.0f));
    p.add (f ("response", "Chord response", 20.0f, 500.0f, 280.0f, 1.0f));
    p.add (f ("pitchLock", "Pitch lock", 0.0f, 100.0f, 30.0f, 1.0f));
    p.add (f ("glide", "Glide", 0.0f, 100.0f, 30.0f, 1.0f));
    p.add (f ("humanize", "Humanize", 0.0f, 100.0f, 35.0f, 1.0f));
    p.add (f ("formant", "Formant", -12.0f, 12.0f, 0.0f, 0.1f));
    p.add (f ("consonants", "Consonants", 0.0f, 100.0f, 60.0f, 1.0f));
    p.add (f ("dry", "Dry", -60.0f, 6.0f, -60.0f, 0.1f));
    p.add (f ("harmony", "Harmony", -60.0f, 6.0f, 0.0f, 0.1f));
    p.add (f ("bypass", "Bypass", 0.0f, 1.0f, 0.0f, 1.0f));
    for (int i = 0; i < 4; ++i)
    {
        p.add (f ("interval" + juce::String (i + 1), "Voice " + juce::String (i + 1) + " interval", -12.0f, 12.0f,
                  std::array<float,4>{ 2.0f, 4.0f, -3.0f, 7.0f}[(size_t)i], 1.0f));
        p.add (f ("level" + juce::String (i + 1), "Voice " + juce::String (i + 1) + " level", -60.0f, 6.0f,
                  std::array<float,4>{0.0f, 2.0f, -8.0f, -9.0f}[(size_t)i], 0.1f));
        p.add (f ("pan" + juce::String (i + 1), "Voice " + juce::String (i + 1) + " pan", -100.0f, 100.0f,
                  std::array<float,4>{-35.0f, 35.0f, 0.0f, 70.0f}[(size_t)i], 1.0f));
    }
    return p;
}

float RrHarmonizerAudioProcessor::getParam (const char* id) const
{
    if (auto* v = parameters.getRawParameterValue (id)) return v->load();
    return 0.0f;
}

void RrHarmonizerAudioProcessor::prepareToPlay (double sr, int)
{
    currentSampleRate = sr > 0.0 ? sr : 44100.0;
    const int delaySize = juce::jmax (4096, (int) currentSampleRate);
    for (auto& s : shifters) s.prepare (delaySize);
    monoHistory.assign (4096, 0.0f);
    for (auto& channelDelay : dryDelay)
        channelDelay.assign (2048, 0.0f);
    dryWritePos = 0;
    setLatencySamples (reportedLatencySamples);
    historyWrite = historyCount = analysisCounter = 0;
    detectedMidi = smoothedMidi = -1.0f;
    envelope = 0.0f;
}
void RrHarmonizerAudioProcessor::releaseResources() {}
bool RrHarmonizerAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet();
    const auto out = l.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && in == out;
}

void RrHarmonizerAudioProcessor::PitchShifter::prepare (int size)
{
    delay.assign ((size_t) juce::jmax (size, 4096), 0.0f);
    writePos = 0;
    phase = 0.0;
}
float RrHarmonizerAudioProcessor::PitchShifter::readLinear (double p) const
{
    const int n = (int) delay.size();
    while (p < 0.0) p += n;
    while (p >= n) p -= n;
    const int a = (int) p, b = (a + 1) % n;
    const float frac = (float) (p - a);
    return delay[(size_t)a] + (delay[(size_t)b] - delay[(size_t)a]) * frac;
}
float RrHarmonizerAudioProcessor::PitchShifter::process (float input, float ratio, double sr)
{
    if (delay.empty()) return input;
    const int n = (int) delay.size();
    delay[(size_t)writePos] = input;
    ratio = juce::jlimit (0.5f, 2.0f, ratio);
    phase += (double) (1.0f - ratio) / 1024.0;
    phase -= std::floor (phase);
    const double phase2 = phase < 0.5 ? phase + 0.5 : phase - 0.5;
    auto head = [&] (double ph)
    {
        const double delaySamples = 32.0 + ph * 1024.0;
        return readLinear ((double)writePos - delaySamples);
    };
    const float a = head (phase), b = head (phase2);
    const float wa = 0.5f - 0.5f * std::cos (2.0f * pi * (float) phase);
    const float wb = 1.0f - wa;
    writePos = (writePos + 1) % n;
    juce::ignoreUnused (sr);
    return a * wa + b * wb;
}

float RrHarmonizerAudioProcessor::detectPitch()
{
    // Normalized autocorrelation on the latest 2048 samples. Runs sparsely on the audio thread.
    const int n = 2048;
    if ((int)monoHistory.size() < n || historyCount < n) return -1.0f;
    float rms = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const int idx = (historyWrite - n + i + (int)monoHistory.size()) % (int)monoHistory.size();
        const float x = monoHistory[(size_t)idx];
        rms += x * x;
    }
    rms = std::sqrt (rms / (float)n);
    const float gateDb = getParam ("gate");
    if (rms < std::pow (10.0f, gateDb / 20.0f)) return -1.0f;

    const int minLag = juce::jmax (2, (int)(currentSampleRate / 1000.0));
    const int maxLag = juce::jmin (n / 2, (int)(currentSampleRate / 65.0));
    float best = 0.0f;
    int bestLag = 0;
    for (int lag = minLag; lag < maxLag; ++lag)
    {
        double corr = 0.0, e1 = 0.0, e2 = 0.0;
        for (int i = 0; i < n - lag; i += 4)
        {
            const int idx = (historyWrite - n + i + (int)monoHistory.size()) % (int)monoHistory.size();
            const float a = monoHistory[(size_t)idx];
            const float b = monoHistory[(size_t)((idx + lag) % (int)monoHistory.size())];
            corr += (double)a * b; e1 += (double)a * a; e2 += (double)b * b;
        }
        const float score = (float)(corr / std::sqrt (e1 * e2 + 1.0e-12));
        if (score > best) { best = score; bestLag = lag; }
    }
    if (best < 0.55f || bestLag <= 0) return -1.0f;
    const float hz = (float)currentSampleRate / (float)bestLag;
    if (hz < 65.0f || hz > 1000.0f) return -1.0f;
    return 69.0f + 12.0f * std::log2 (hz / 440.0f);
}

float RrHarmonizerAudioProcessor::quantizeMidi (float midi, int key, int scale) const
{
    static const std::array<std::array<int,7>,5> patterns {{
        {{0,2,4,5,7,9,11}}, {{0,2,3,5,7,8,10}}, {{0,2,3,5,7,8,11}},
        {{0,2,3,5,7,9,10}}, {{0,2,4,5,7,9,10}}
    }};
    if (scale == 5) return midi;
    const int note = (int)std::lround (midi);
    const int rel = ((note - key) % 12 + 12) % 12;
    int nearest = 0, dist = 99;
    const int count = 7;
    for (int i = 0; i < count; ++i)
    {
        const int candidate = patterns[(size_t)juce::jlimit (0,4,scale)][(size_t)i];
        const int d = std::abs (rel - candidate);
        const int wrapped = juce::jmin (d, 12 - d);
        if (wrapped < dist) { dist = wrapped; nearest = candidate; }
    }
    return (float)(note - rel + nearest);
}

void RrHarmonizerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midi);
    const int channels = buffer.getNumChannels(), samples = buffer.getNumSamples();
    if (channels == 0) return;
    const bool bypass = getParam ("bypass") >= 0.5f;
    const int voices = (int)getParam ("voices") + 1;
    const int mode = (int)getParam ("mode");
    const int key = (int)getParam ("key");
    const int scale = (int)getParam ("scale");
    const float dryGain = juce::Decibels::decibelsToGain (getParam ("dry"));
    const float harmonyGain = juce::Decibels::decibelsToGain (getParam ("harmony"));
    const float glide = getParam ("glide") / 100.0f;
    std::array<float, maxVoices> baseIntervals {}, voiceLevelGains {}, voicePanValues {};
    for (int v = 0; v < maxVoices; ++v)
    {
        baseIntervals[(size_t)v] = getParam (("interval" + juce::String(v + 1)).toRawUTF8());
        voiceLevelGains[(size_t)v] = juce::Decibels::decibelsToGain (getParam (("level" + juce::String(v + 1)).toRawUTF8()));
        voicePanValues[(size_t)v] = getParam (("pan" + juce::String(v + 1)).toRawUTF8()) / 100.0f;
    }
    for (int i = 0; i < samples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < channels; ++ch) mono += buffer.getReadPointer(ch)[i];
        mono /= (float)channels;
        envelope = 0.999f * envelope + 0.001f * std::abs (mono);
        monoHistory[(size_t)historyWrite] = mono;
        historyWrite = (historyWrite + 1) % (int)monoHistory.size();
        historyCount = juce::jmin (historyCount + 1, (int)monoHistory.size());
        if (++analysisCounter >= 1024)
        {
            analysisCounter = 0;
            detectedMidi = detectPitch();
            if (detectedMidi >= 0.0f)
            {
                const float quantized = quantizeMidi (detectedMidi, key, scale);
                const float lock = getParam ("pitchLock") / 100.0f;
                const float target = mode == 0 ? detectedMidi + (quantized - detectedMidi) * lock : detectedMidi;
                if (smoothedMidi < 0.0f) smoothedMidi = target;
                const float responseMs = getParam ("response");
                const float responseCoeff = 1.0f - std::exp (-1024.0f / (float)(currentSampleRate * juce::jmax (0.02f, responseMs / 1000.0f)));
                smoothedMidi += (target - smoothedMidi) * juce::jlimit (0.01f, 1.0f, responseCoeff);
            }
        }
        std::array<float, maxVoices> shiftedVoices {};
        std::array<float, maxVoices> voiceLevels {};
        std::array<float, maxVoices> voicePans {};
        for (int v = 0; v < voices; ++v)
        {
            float semitones = baseIntervals[(size_t)v];
            if (detectedMidi >= 0.0f && smoothedMidi >= 0.0f && mode != 2)
            {
                const float source = smoothedMidi;
                const float rawTarget = source + semitones;
                const float target = quantizeMidi (rawTarget, key, scale);
                if (mode == 0)
                {
                    // Scale mode blends the harmony note toward the nearest in-scale note.
                    const float lock = getParam ("pitchLock") / 100.0f;
                    semitones += (target - rawTarget) * lock;
                }
                else if (mode == 1)
                {
                    // Chord mode snaps each generated harmony note fully to the selected scale.
                    semitones = target - source;
                }
            }
            const float ratio = std::pow (2.0f, semitones / 12.0f);
            shiftedVoices[(size_t)v] = shifters[(size_t)v].process (mono, ratio, currentSampleRate);
            voiceLevels[(size_t)v] = voiceLevelGains[(size_t)v];
            voicePans[(size_t)v] = voicePanValues[(size_t)v];
        }
        for (int ch = 0; ch < channels; ++ch)
        {
            const float input = buffer.getReadPointer(ch)[i];
            auto& delay = dryDelay[(size_t)juce::jmin (ch, 1)];
            delay[(size_t)dryWritePos] = input;
            const int readPos = (dryWritePos - reportedLatencySamples + (int)delay.size()) % (int)delay.size();
            const float delayedInput = delay[(size_t)readPos];
            float wet = 0.0f;
            for (int v = 0; v < voices; ++v)
            {
                const float pan = voicePans[(size_t)v];
                const float panGain = channels == 1 ? 1.0f : (ch == 0 ? std::sqrt (0.5f * (1.0f - pan)) : std::sqrt (0.5f * (1.0f + pan)));
                wet += shiftedVoices[(size_t)v] * voiceLevels[(size_t)v] * panGain;
            }
            const float out = bypass ? delayedInput : delayedInput * dryGain + wet * harmonyGain;
            buffer.getWritePointer(ch)[i] = std::isfinite(out) ? out : 0.0f;
        }
        dryWritePos = (dryWritePos + 1) % (int)dryDelay[0].size();
    }
}

void RrHarmonizerAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, dest);
}
void RrHarmonizerAudioProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* RrHarmonizerAudioProcessor::createEditor()
{
    return new RrHarmonizerAudioProcessorEditor (*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RrHarmonizerAudioProcessor();
}
