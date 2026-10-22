
#include "PluginEditor.h"

RrHarmonizerAudioProcessorEditor::RrHarmonizerAudioProcessorEditor
    (RrHarmonizerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1000, 680);
    setResizable (true, true);

    title.setText ("Rr Harmonizer", juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centredLeft);
    title.setFont (juce::FontOptions (25.0f, "Bold"));
    addAndMakeVisible (title);

    reset.setButtonText ("RESET");
    addAndMakeVisible (reset);

    bypass.setButtonText ("BYPASS");
    addAndMakeVisible (bypass);

    reset.onClick = [this]
    {
        for (auto* parameter : processor.getParameters())
        {
            if (auto* ranged =
                dynamic_cast<juce::RangedAudioParameter*> (parameter))
            {
                ranged->setValueNotifyingHost (ranged->getDefaultValue());
            }
        }
    };

    bypass.onClick = [this]
    {
        if (auto* parameter =
            processor.parameters.getParameter ("bypass"))
        {
            const bool isBypassed = parameter->getValue() >= 0.5f;
            parameter->setValueNotifyingHost (
                isBypassed ? 0.0f : 1.0f);
        }
    };

    setWantsKeyboardFocus (true);
}

RrHarmonizerAudioProcessorEditor::~RrHarmonizerAudioProcessorEditor()
{
}

void RrHarmonizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff101319));

    g.setColour (juce::Colour (0xff1b2029));
    g.fillRoundedRectangle (12.0f, 12.0f,
                            (float) getWidth() - 24.0f, 58.0f, 8.0f);

    g.setColour (juce::Colour (0xff242b36));
    const int top = 86;
    const int panelHeight = getHeight() - 190;
    const int gap = 12;
    const int panelWidth = (getWidth() - 36 - 4 * gap) / 5;

    const juce::StringArray names {
        "INPUT", "HARMONY", "VOICES", "TONE", "OUTPUT"
    };

    for (int i = 0; i < names.size(); ++i)
    {
        const float x = 12.0f + i * (panelWidth + gap);
        g.fillRoundedRectangle (x, (float) top,
                                (float) panelWidth,
                                (float) panelHeight, 8.0f);
        g.setColour (juce::Colour (0xffe3b86b));
        g.setFont (juce::FontOptions (14.0f, "Bold"));
        g.drawText (names[i], (int) x + 12, top + 12,
                    panelWidth - 24, 24,
                    juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xff242b36));
    }

    g.setColour (juce::Colour (0xff1b2029));
    g.fillRoundedRectangle (12.0f, (float) getHeight() - 88.0f,
                            (float) getWidth() - 24.0f, 76.0f, 8.0f);

    g.setColour (juce::Colour (0xff8f9aaa));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("PITCH LOCK     GLIDE     HUMANIZE     FORMANT"
                "     CONSONANTS     DRY     HARMONY",
                26, getHeight() - 64, getWidth() - 52, 28,
                juce::Justification::centredLeft);
}

void RrHarmonizerAudioProcessorEditor::resized()
{
    title.setBounds (24, 24, getWidth() - 240, 34);
    reset.setBounds (getWidth() - 190, 25, 76, 30);
    bypass.setBounds (getWidth() - 104, 25, 80, 30);
}
