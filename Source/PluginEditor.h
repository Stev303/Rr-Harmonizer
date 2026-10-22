
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <array>
#include <memory>
#include <vector>

class RrHarmonizerAudioProcessorEditor final
    : public juce::AudioProcessorEditor
{
public:
    explicit RrHarmonizerAudioProcessorEditor (
        RrHarmonizerAudioProcessor& processor);

    ~RrHarmonizerAudioProcessorEditor() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    using ComboAttachment =
        juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    RrHarmonizerAudioProcessor& processor;

    // Headings and readouts
    juce::Label title;
    juce::Label pitchTab;

    juce::Label sectionInput;
    juce::Label sectionHarmony;
    juce::Label sectionVoices;
    juce::Label sectionTone;
    juce::Label sectionOutput;

    juce::Label pitchReadout;
    juce::Label statusReadout;

    juce::Label noteHeading;
    juce::Label levelHeading;
    juce::Label panHeading;
    juce::Label outHeading;

    juce::Label noPitch;
    juce::Label chordSourceLabel;

    // Main selectors
    juce::ComboBox key;
    juce::ComboBox scale;
    juce::ComboBox source;

    // Utility buttons
    juce::TextButton undo { "Undo" };
    juce::TextButton redo { "Redo" };
    juce::TextButton copy { "Copy" };
    juce::TextButton paste { "Paste" };
    juce::TextButton reset { "Reset" };
    juce::TextButton bypass { "Bypass" };
    juce::TextButton resetWindow { "Reset Window" };

    // Harmony mode buttons
    std::array<juce::TextButton, 3> modeButtons;

    juce::TextButton manualButton { "MANUAL" };
    juce::TextButton autoButton { "AUTO" };
    juce::TextButton sourcePlus { "+" };

    // Latency / pitch range
    std::array<juce::TextButton, 3> latencyButtons;

    // Number of harmony voices
    std::array<juce::TextButton, 4> countButtons;

    // Per-voice interval controls
    std::array<juce::TextButton, 4> intervalDown;
    std::array<juce::TextButton, 4> intervalUp;

    // Global controls
    juce::Slider gate;
    juce::Slider response;
    juce::Slider pitchLock;
    juce::Slider glide;
    juce::Slider humanize;
    juce::Slider formant;
    juce::Slider consonants;
    juce::Slider dry;
    juce::Slider harmony;

    // Per-voice controls
    std::array<juce::Slider, 4> interval;
    std::array<juce::Slider, 4> level;
    std::array<juce::Slider, 4> pan;

    // Per-voice labels
    std::array<juce::Label, 4> voiceLabels;
    std::array<juce::Label, 4> intervalLabels;
    std::array<juce::Label, 4> noteLabels;

    // Attachments for parameter synchronization
    std::array<std::unique_ptr<SliderAttachment>, 21>
        sliderAttachments;

    std::array<std::unique_ptr<ComboAttachment>, 3>
        comboAttachments;

    // UI helper functions
    void setupSlider (
        juce::Slider& slider,
        const juce::String& parameterID,
        const juce::String& label,
        bool rotary = true);

    void setupCombo (
        juce::ComboBox& combo,
        const juce::String& parameterID,
        const juce::StringArray& items);

    void styleButton (
        juce::TextButton& button,
        bool selected = false);

    // Parameter and preset actions
    void resetAllParameters();
    void updateModeButtons();
    void updateVoiceButtons();
    void updateLatencyButtons();
    void updateSourceButtons();
    void updatePitchReadout();

    // Undo / redo state
    void saveUndoState();
    void performUndo();
    void performRedo();

    std::vector<juce::ValueTree> undoHistory;
    std::vector<juce::ValueTree> redoHistory;

    bool restoringState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
        RrHarmonizerAudioProcessorEditor)
};
