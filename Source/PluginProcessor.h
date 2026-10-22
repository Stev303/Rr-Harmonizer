#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

class RrHarmonizerAudioProcessor final : public juce::AudioProcessor
{
public:
    RrHarmonizerAudioProcessor();
    ~RrHarmonizerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.25; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
    static APVTS::ParameterLayout createParameterLayout();

private:
    static constexpr int maxVoices = 4;
    struct PitchShifter
    {
        std::vector<float> delay;
        int writePos = 0;
        double phase = 0.0;
        float process (float input, float ratio, double sr);
        void prepare (int size);
        float readLinear (double position) const;
    };
    std::array<PitchShifter, maxVoices> shifters;
    std::vector<float> monoHistory;
    std::array<std::vector<float>, 2> dryDelay;
    int dryWritePos = 0;
    static constexpr int reportedLatencySamples = 544;
    int historyWrite = 0;
    int historyCount = 0;
    double currentSampleRate = 44100.0;
    int analysisCounter = 0;
    float detectedMidi = -1.0f;
    float smoothedMidi = -1.0f;
    float envelope = 0.0f;
    juce::Random random;

    float detectPitch();
    float quantizeMidi (float midi, int key, int scale) const;
    float getParam (const char* id) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RrHarmonizerAudioProcessor)
};
