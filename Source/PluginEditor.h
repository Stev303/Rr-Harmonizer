#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class RrHarmonizerAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit RrHarmonizerAudioProcessorEditor (RrHarmonizerAudioProcessor&);
    ~RrHarmonizerAudioProcessorEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    RrHarmonizerAudioProcessor& processor;

    juce::Label title, pitchTab, sectionInput, sectionHarmony, sectionVoices, sectionTone, sectionOutput;
    juce::Label pitchReadout, statusReadout, noteHeading, levelHeading, panHeading, outHeading;
    juce::Label noPitch, chordSourceLabel;
    juce::ComboBox key, scale, source;
    juce::TextButton undo {"Undo"}, redo {"Redo"}, copy {"Copy"}, paste {"Paste"}, reset {"Reset"}, bypass {"Bypass"}, resetWindow {"Reset Window"};
    std::array<juce::TextButton,3> modeButtons;
    juce::TextButton manualButton {"MANUAL"}, autoButton {"AUTO"}, sourcePlus {"+"};
    std::array<juce::TextButton,3> latencyButtons;
    std::array<juce::TextButton,4> countButtons;
    std::array<juce::TextButton,4> intervalDown, intervalUp;
    juce::Slider gate, response, pitchLock, glide, humanize, formant, consonants, dry, harmony;
    std::array<juce::Slider,4> interval, level, pan;
    std::array<juce::Label,4> voiceLabels, intervalLabels, noteLabels;
    std::array<std::unique_ptr<SliderAttachment>,21> sliderAttachments;
    std::array<std::unique_ptr<ComboAttachment>,3> comboAttachments;
    void setupSlider (juce::Slider&, const juce::String&, const juce::String&, bool rotary = true);
    void setupCombo (juce::ComboBox&, const juce::String&, const juce::StringArray&);
    void styleButton (juce::TextButton&, bool selected = false);
    void resetAllParameters();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RrHarmonizerAudioProcessorEditor)
};
