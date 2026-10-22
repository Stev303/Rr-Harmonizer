
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
    constexpr float pi = 3.14159265358979323846f;

    const juce::StringArray noteNames
    {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
}

//==============================================================
// Constructor
//==============================================================

RrHarmonizerAudioProcessor::RrHarmonizerAudioProcessor()
    : AudioProcessor (
        BusesProperties()
            .withInput ("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

//==============================================================
// Parameter layout
//==============================================================

RrHarmonizerAudioProcessor::APVTS::ParameterLayout
RrHarmonizerAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout p;

    auto choice = [] (
        const juce::String& id,
        const juce::String& name,
        juce::StringArray options,
        int def)
    {
        return std::make_unique<juce::AudioParameterChoice>(
            id, name, options, def);
    };

    auto f = [] (
        const juce::String& id,
        const juce::String& name,
        float lo,
        float hi,
        float def,
        float step = 0.0f)
    {
        juce::NormalisableRange<float> range (lo, hi, step);

        return std::make_unique<juce::AudioParameterFloat>(
            id, name, range, def);
    };

    p.add (choice ("mode", "Harmony mode",
                   { "Scale", "Chord", "Fixed" }, 0));

    p.add (choice ("key", "Key", noteNames, 1));

    p.add (choice ("scale", "Scale",
                   { "Major", "Natural Minor", "Harmonic Minor",
                     "Dorian", "Mixolydian", "Chromatic" }, 0));

    p.add (choice ("source", "Chord source",
                   { "Music", "Manual" }, 0));

    p.add (choice ("voices", "Voice count",
                   { "1", "2", "3", "4" }, 2));

    p.add (choice ("latency", "Pitch range",
                   { "Low", "Mid", "High" }, 1));

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

    const std::array<float, 4> defaultIntervals { 2.0f, 4.0f, -3.0f, 7.0f };
    const std::array<float, 4> defaultLevels    { 0.0f, 2.0f, -8.0f, -9.0f };
    const std::array<float, 4> defaultPans      { -35.0f, 35.0f, 0.0f, 70.0f };

    for (int i = 0; i < 4; ++i)
    {
        const auto number = juce::String (i + 1);

        p.add (f ("interval" + number,
                  "Voice " + number + " interval",
                  -12.0f, 12.0f, defaultIntervals[(size_t) i], 1.0f));

        p.add (f ("level" + number,
                  "Voice " + number + " level",
                  -60.0f, 6.0f, defaultLevels[(size_t) i], 0.1f));

        p.add (f ("pan" + number,
                  "Voice " + number + " pan",
                  -100.0f, 100.0f, defaultPans[(size_t) i], 1.0f));
    }

    return p;
}

//==============================================================
// Parameter helper
//==============================================================

float RrHarmonizerAudioProcessor::getParam (const char* id) const
{
    if (auto* value = parameters.getRawParameterValue (id))
        return value->load (std::memory_order_relaxed);

    return 0.0f;
}

//==============================================================
// Audio setup
//==============================================================

void RrHarmonizerAudioProcessor::prepareToPlay (
    double sampleRate,
    int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    const int delaySize = juce::jmax (
        4096, (int) currentSampleRate);

    for (auto& shifter : shifters)
        shifter.prepare (delaySize);

    monoHistory.assign (4096, 0.0f);

    for (auto& channelDelay : dryDelay)
        channelDelay.assign (2048, 0.0f);

    dryWritePos = 0;
    historyWrite = 0;
    historyCount = 0;
    analysisCounter = 0;

    detectedMidi = -1.0f;
    smoothedMidi = -1.0f;
    envelope = 0.0f;

    detectedMidiForUi.store (-1.0f, std::memory_order_relaxed);
    outputLevelForUi.store (0.0f, std::memory_order_relaxed);

    setLatencySamples (reportedLatencySamples);
}

void RrHarmonizerAudioProcessor::releaseResources()
{
}

bool RrHarmonizerAudioProcessor::isBusesLayoutSupported (
    const BusesLayout& layouts) const
{
    const auto input  = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    return (input == juce::AudioChannelSet::mono()
         || input == juce::AudioChannelSet::stereo())
         && input == output;
}

//==============================================================
// Pitch shifter
//==============================================================

void RrHarmonizerAudioProcessor::PitchShifter::prepare (int size)
{
    delay.assign ((size_t) juce::jmax (size, 4096), 0.0f);

    writePos = 0;
    phase = 0.0;
}

float RrHarmonizerAudioProcessor::PitchShifter::readLinear (
    double position) const
{
    const int size = (int) delay.size();

    if (size == 0)
        return 0.0f;

    while (position < 0.0)
        position += size;

    while (position >= size)
        position -= size;

    const int a = (int) position;
    const int b = (a + 1) % size;
    const float fraction = (float) (position - a);

    return delay[(size_t) a]
         + (delay[(size_t) b] - delay[(size_t) a]) * fraction;
}

float RrHarmonizerAudioProcessor::PitchShifter::process (
    float input,
    float ratio,
    double sampleRate)
{
    juce::ignoreUnused (sampleRate);

    if (delay.empty())
        return input;

    const int size = (int) delay.size();

    delay[(size_t) writePos] = input;

    ratio = juce::jlimit (0.5f, 2.0f, ratio);

    phase += (double) (1.0f - ratio) / 1024.0;
    phase -= std::floor (phase);

    const double phase2 = phase < 0.5
                        ? phase + 0.5
                        : phase - 0.5;

    auto readHead = [this, &size] (double ph)
    {
        const double delaySamples = 32.0 + ph * 1024.0;
        return readLinear ((double) writePos - delaySamples);
    };

    const float a = readHead (phase);
    const float b = readHead (phase2);

    const float weightA =
        0.5f - 0.5f * std::cos (2.0f * pi * (float) phase);

    const float weightB = 1.0f - weightA;

    writePos = (writePos + 1) % size;

    return a * weightA + b * weightB;
}

//==============================================================
// Pitch detection
//==============================================================

float RrHarmonizerAudioProcessor::detectPitch()
{
    constexpr int windowSize = 2048;

    if ((int) monoHistory.size() < windowSize
        || historyCount < windowSize)
        return -1.0f;

    float rms = 0.0f;

    for (int i = 0; i < windowSize; ++i)
    {
        const int index =
            (historyWrite - windowSize + i
             + (int) monoHistory.size())
            % (int) monoHistory.size();

        const float sample = monoHistory[(size_t) index];
        rms += sample * sample;
    }

    rms = std::sqrt (rms / (float) windowSize);

    const float gateDb = getParam ("gate");

    if (rms < std::pow (10.0f, gateDb / 20.0f))
        return -1.0f;

    const int minLag = juce::jmax (
        2, (int) (currentSampleRate / 1000.0));

    const int maxLag = juce::jmin (
        windowSize / 2, (int) (currentSampleRate / 65.0));

    float bestScore = 0.0f;
    int bestLag = 0;

    for (int lag = minLag; lag < maxLag; ++lag)
    {
        double correlation = 0.0;
        double energyA = 0.0;
        double energyB = 0.0;

        for (int i = 0; i < windowSize - lag; i += 4)
        {
            const int index =
                (historyWrite - windowSize + i
                 + (int) monoHistory.size())
                % (int) monoHistory.size();

            const float a = monoHistory[(size_t) index];

            const float b = monoHistory[
                (size_t) ((index + lag) % (int) monoHistory.size())];

            correlation += (double) a * b;
            energyA += (double) a * a;
            energyB += (double) b * b;
        }

        const float score = (float) (
            correlation / std::sqrt (energyA * energyB + 1.0e-12));

        if (score > bestScore)
        {
            bestScore = score;
            bestLag = lag;
        }
    }

    if (bestScore < 0.55f || bestLag <= 0)
        return -1.0f;

    const float frequency = (float) currentSampleRate / (float) bestLag;

    if (frequency < 65.0f || frequency > 1000.0f)
        return -1.0f;

    return 69.0f + 12.0f * std::log2 (frequency / 440.0f);
}

//==============================================================
// Scale quantization
//==============================================================

float RrHarmonizerAudioProcessor::quantizeMidi (
    float midi,
    int key,
    int scale) const
{
    static const std::array<std::array<int, 7>, 5> patterns
    {{
        {{ 0, 2, 4, 5, 7, 9, 11 }},  // Major
        {{ 0, 2, 3, 5, 7, 8, 10 }},  // Natural minor
        {{ 0, 2, 3, 5, 7, 8, 11 }},  // Harmonic minor
        {{ 0, 2, 3, 5, 7, 9, 10 }},  // Dorian
        {{ 0, 2, 4, 5, 7, 9, 10 }}   // Mixolydian
    }};

    if (scale == 5)
        return midi;

    const int note = (int) std::lround (midi);
    const int relativeNote = ((note - key) % 12 + 12) % 12;

    int nearestNote = 0;
    int nearestDistance = 99;

    for (int i = 0; i < 7; ++i)
    {
        const int candidate =
            patterns[(size_t) juce::jlimit (0, 4, scale)][(size_t) i];

        const int distance = std::abs (relativeNote - candidate);
        const int wrappedDistance = juce::jmin (distance, 12 - distance);

        if (wrappedDistance < nearestDistance)
        {
            nearestDistance = wrappedDistance;
            nearestNote = candidate;
        }
    }

    return (float) (note - relativeNote + nearestNote);
}

//==============================================================
// Main audio processing
//==============================================================

void RrHarmonizerAudioProcessor::processBlock (
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    if (channels == 0 || monoHistory.empty() || dryDelay[0].empty())
        return;

    const bool bypass = getParam ("bypass") >= 0.5f;

    const int voices = juce::jlimit (
        1, maxVoices, (int) getParam ("voices") + 1);

    const int mode  = (int) getParam ("mode");
    const int key   = (int) getParam ("key");
    const int scale = (int) getParam ("scale");

    const float dryGain =
        juce::Decibels::decibelsToGain (getParam ("dry"));

    const float harmonyGain =
        juce::Decibels::decibelsToGain (getParam ("harmony"));

    std::array<float, maxVoices> intervals {};
    std::array<float, maxVoices> levelGains {};
    std::array<float, maxVoices> panValues {};

    for (int v = 0; v < maxVoices; ++v)
    {
        const auto number = juce::String (v + 1);

        intervals[(size_t) v] =
            getParam (("interval" + number).toRawUTF8());

        levelGains[(size_t) v] =
            juce::Decibels::decibelsToGain (
                getParam (("level" + number).toRawUTF8()));

        panValues[(size_t) v] =
            getParam (("pan" + number).toRawUTF8()) / 100.0f;
    }

    float blockPeak = 0.0f;

    for (int i = 0; i < samples; ++i)
    {
        float mono = 0.0f;

        for (int ch = 0; ch < channels; ++ch)
            mono += buffer.getReadPointer (ch)[i];

        mono /= (float) channels;

        envelope = 0.999f * envelope
                 + 0.001f * std::abs (mono);

        monoHistory[(size_t) historyWrite] = mono;

        historyWrite = (historyWrite + 1) % (int) monoHistory.size();

        historyCount = juce::jmin (
            historyCount + 1, (int) monoHistory.size());

        if (++analysisCounter >= 1024)
        {
            analysisCounter = 0;
            detectedMidi = detectPitch();

            if (detectedMidi >= 0.0f)
            {
                const float quantized =
                    quantizeMidi (detectedMidi, key, scale);

                const float pitchLock = getParam ("pitchLock") / 100.0f;

                const float target = mode == 0
                    ? detectedMidi + (quantized - detectedMidi) * pitchLock
                    : detectedMidi;

                if (smoothedMidi < 0.0f)
                    smoothedMidi = target;

                const float responseMs = getParam ("response");

                const float responseCoefficient =
                    1.0f - std::exp (
                        -1024.0f /
                        (float) (currentSampleRate
                        * juce::jmax (0.02f, responseMs / 1000.0f)));

                smoothedMidi +=
                    (target - smoothedMidi)
                    * juce::jlimit (0.01f, 1.0f, responseCoefficient);
            }

            detectedMidiForUi.store (
                detectedMidi, std::memory_order_relaxed);
        }

        std::array<float, maxVoices> shiftedVoices {};

        for (int v = 0; v < voices; ++v)
        {
            float semitones = intervals[(size_t) v];

            if (detectedMidi >= 0.0f
                && smoothedMidi >= 0.0f
                && mode != 2)
            {
                const float sourceNote = smoothedMidi;
                const float rawTarget = sourceNote + semitones;

                const float target =
                    quantizeMidi (rawTarget, key, scale);

                if (mode == 0)
                {
                    const float pitchLock =
                        getParam ("pitchLock") / 100.0f;

                    semitones += (target - rawTarget) * pitchLock;
                }
                else if (mode == 1)
                {
                    semitones = target - sourceNote;
                }
            }

            const float ratio = std::pow (2.0f, semitones / 12.0f);

            shiftedVoices[(size_t) v] =
                shifters[(size_t) v].process (
                    mono, ratio, currentSampleRate);
        }

        for (int ch = 0; ch < channels; ++ch)
        {
            const float input = buffer.getReadPointer (ch)[i];

            auto& delay = dryDelay[(size_t) juce::jmin (ch, 1)];

            delay[(size_t) dryWritePos] = input;

            const int readPosition =
                (dryWritePos - reportedLatencySamples
                 + (int) delay.size())
                % (int) delay.size();

            const float delayedInput = delay[(size_t) readPosition];

            float wet = 0.0f;

            for (int v = 0; v < voices; ++v)
            {
                const float pan = panValues[(size_t) v];

                const float panGain = channels == 1
                    ? 1.0f
                    : (ch == 0
                        ? std::sqrt (0.5f * (1.0f - pan))
                        : std::sqrt (0.5f * (1.0f + pan)));

                wet += shiftedVoices[(size_t) v]
                     * levelGains[(size_t) v]
                     * panGain;
            }

            const float output = bypass
                ? delayedInput
                : delayedInput * dryGain + wet * harmonyGain;

            const float safeOutput =
                std::isfinite (output) ? output : 0.0f;

            buffer.getWritePointer (ch)[i] = safeOutput;

            blockPeak = juce::jmax (
                blockPeak, std::abs (safeOutput));
        }
    }

    // Publish the output level for the editor's meter.
    outputLevelForUi.store (
        blockPeak, std::memory_order_relaxed);

    dryWritePos = (dryWritePos + samples) % (int) dryDelay[0].size();
}

//==============================================================
// Save and restore state
//==============================================================

void RrHarmonizerAudioProcessor::getStateInformation (
    juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();

    std::unique_ptr<juce::XmlElement> xml (state.createXml());

    if (xml != nullptr)
        copyXmlToBinary (*xml, destination);
}

void RrHarmonizerAudioProcessor::setStateInformation (
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (
        getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr
        && xml->hasTagName (parameters.state.getType()))
    {
        parameters.replaceState (
            juce::ValueTree::fromXml (*xml));
    }
}

//==============================================================
// Editor and plugin entry point
//==============================================================

juce::AudioProcessorEditor*
RrHarmonizerAudioProcessor::createEditor()
{
    return new RrHarmonizerAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RrHarmonizerAudioProcessor();
}
