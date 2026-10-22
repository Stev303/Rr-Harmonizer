
#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

class RrHarmonizerAudioProcessor final : public juce::AudioProcessor
{
public:
    RrHarmonizerAudioProcessor();
    ~RrHarmonizerAudioProcessor() override = default;

    // Audio processing
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (
        juce::AudioBuffer<float>& buffer,
        juce::MidiBuffer& midiMessages) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Plugin information
    const juce::String getName() const override
    {
        return JucePlugin_Name;
    }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override
    {
        return 0.25;
    }

    // Programs
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }

    void setCurrentProgram (int) override {}

    const juce::String getProgramName (int) override
    {
        return {};
    }

    void changeProgramName (
        int,
        const juce::String&) override
    {
    }

    // Save and restore plugin state
    void getStateInformation (juce::MemoryBlock& destData) override;

    void setStateInformation (
        const void* data,
        int sizeInBytes) override;

    // Parameter system
    using APVTS = juce::AudioProcessorValueTreeState;

    APVTS parameters;

    static APVTS::ParameterLayout createParameterLayout();

    // Values for the editor's pitch and output displays.
    // The audio thread updates these values; the editor reads them.
    float getDetectedMidi() const noexcept
    {
        return detectedMidiForUi.load (std::memory_order_relaxed);
    }

    float getOutputLevel() const noexcept
    {
        return outputLevelForUi.load (std::memory_order_relaxed);
    }

private:
    static constexpr int maxVoices = 4;

    // Simple pitch-shifter state for each harmony voice.
    struct PitchShifter
    {
        std::vector<float> delay;

        int writePos = 0;
        double phase = 0.0;

        void prepare (int size);

        float readLinear (double position) const;

        float process (
            float input,
            float ratio,
            double sampleRate);
    };

    std::array<PitchShifter, maxVoices> shifters;

    // Mono history used by pitch detection.
    std::vector<float> monoHistory;

    // Delayed dry signal, matched to the harmonizer's processing latency.
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

    // Thread-safe values for the editor.
    std::atomic<float> detectedMidiForUi { -1.0f };
    std::atomic<float> outputLevelForUi { 0.0f };

    // Internal audio-analysis functions.
    float detectPitch();

    float quantizeMidi (
        float midi,
        int key,
        int scale) const;

    float getParam (const char* id) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
        RrHarmonizerAudioProcessor)
};
