#include "PluginEditor.h"

namespace
{
const juce::Colour bg       (0xff38383d);
const juce::Colour panel    (0xff2d2d32);
const juce::Colour panelDark(0xff19191d);
const juce::Colour header   (0xff202025);
const juce::Colour lime     (0xffb4d936);
const juce::Colour text     (0xffd5d5d9);
const juce::Colour muted    (0xff8e8e96);
const juce::Colour button   (0xff505057);
const juce::Colour selected (0xff536b8d);
}

RrHarmonizerAudioProcessorEditor::RrHarmonizerAudioProcessorEditor (
    RrHarmonizerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (950, 410);

    auto label = [this] (juce::Label& l, const juce::String& s,
                         float size = 11.0f, bool bold = false)
    {
        addAndMakeVisible (l);
        l.setText (s, juce::dontSendNotification);
        l.setColour (juce::Label::textColourId, text);
        l.setFont (juce::Font (
            juce::FontOptions (size).withStyle (bold ? "Bold" : "Plain")));
    };

    // Header and section titles.
    label (title, "Harmonizer", 13.0f, true);
    label (pitchTab, "Pitch", 10.0f);
    label (sectionInput, "I N P U T", 8.0f, true);
    label (sectionHarmony, "H A R M O N Y", 8.0f, true);
    label (sectionVoices, "V O I C E S", 8.0f, true);
    label (sectionTone, "T O N E", 8.0f, true);
    label (sectionOutput, "O U T P U T", 8.0f, true);

    label (pitchReadout, "N.C.", 21.0f, true);
    label (statusReadout, "Key: C# major  (auto pitch detection)", 9.0f);
    label (noPitch, "no pitch", 8.0f);

    label (chordSourceLabel, "CHORD SOURCE", 8.0f, true);
    label (noteHeading, "NOTE", 8.0f, true);
    label (levelHeading, "LEVEL", 8.0f, true);
    label (panHeading, "PAN", 8.0f, true);
    label (outHeading, "OUT", 8.0f, true);

    // Header buttons.
    for (auto* b : { &undo, &redo, &copy, &paste, &reset,
                     &bypass, &resetWindow })
    {
        addAndMakeVisible (*b);
        styleButton (*b);
    }

    // Undo/redo are deliberately disabled until an undo history is implemented.
    undo.setEnabled (false);
    redo.setEnabled (false);

    // Harmony mode.
    const juce::StringArray modeNames { "SCALE", "CHORD", "FIXED" };

    for (int i = 0; i < 3; ++i)
    {
        auto& b = modeButtons[(size_t) i];
        b.setButtonText (modeNames[i]);
        addAndMakeVisible (b);
        styleButton (b, i == 0);

        b.onClick = [this, i]
        {
            if (auto* param = processor.parameters.getParameter ("mode"))
                param->setValueNotifyingHost ((float) i / 2.0f);

            for (int j = 0; j < 3; ++j)
                styleButton (modeButtons[(size_t) j], j == i);
        };
    }

    // Manual/automatic selection and chord-source controls.
    for (auto* b : { &manualButton, &autoButton, &sourcePlus })
    {
        addAndMakeVisible (*b);
        styleButton (*b, b == &autoButton);
    }

    manualButton.onClick = [this]
    {
        styleButton (manualButton, true);
        styleButton (autoButton, false);
    };

    autoButton.onClick = [this]
    {
        styleButton (manualButton, false);
        styleButton (autoButton, true);
    };

    sourcePlus.onClick = [this]
    {
        const int current = source.getSelectedItemIndex();
        source.setSelectedItemIndex (current == 0 ? 1 : 0);
    };

    // Parameter-linked combo boxes.
    setupCombo (key, "key",
                { "C", "C#", "D", "D#", "E", "F",
                  "F#", "G", "G#", "A", "A#", "B" });

    setupCombo (scale, "scale",
                { "Major", "Natural Minor", "Harmonic Minor",
                  "Dorian", "Mixolydian", "Chromatic" });

    setupCombo (source, "source", { "Music", "Manual" });

    comboAttachments[0] =
        std::make_unique<ComboAttachment> (processor.parameters, "key", key);

    comboAttachments[1] =
        std::make_unique<ComboAttachment> (processor.parameters, "scale", scale);

    comboAttachments[2] =
        std::make_unique<ComboAttachment> (processor.parameters, "source", source);

    // Latency buttons.
    const juce::StringArray latencyNames { "LOW", "MID", "HIGH" };

    for (int i = 0; i < 3; ++i)
    {
        auto& b = latencyButtons[(size_t) i];
        b.setButtonText (latencyNames[i]);
        addAndMakeVisible (b);
        styleButton (b, i == 1);

        b.onClick = [this, i]
        {
            if (auto* param = processor.parameters.getParameter ("latency"))
                param->setValueNotifyingHost ((float) i / 2.0f);

            for (int j = 0; j < 3; ++j)
                styleButton (latencyButtons[(size_t) j], j == i);
        };
    }

    // Voice count, interval buttons and voice-row labels.
    const juce::StringArray intervalNames {
        "3rd above", "5th above", "4th below", "Octave above"
    };

    for (int i = 0; i < 4; ++i)
    {
        auto& count = countButtons[(size_t) i];
        count.setButtonText (juce::String (i + 1));
        addAndMakeVisible (count);
        styleButton (count, i == 2);

        count.onClick = [this, i]
        {
            if (auto* param = processor.parameters.getParameter ("voices"))
                param->setValueNotifyingHost ((float) i / 3.0f);

            for (int j = 0; j < 4; ++j)
                styleButton (countButtons[(size_t) j], j == i);
        };

        auto& down = intervalDown[(size_t) i];
        auto& up   = intervalUp[(size_t) i];

        down.setButtonText ("-");
        up.setButtonText ("+");

        addAndMakeVisible (down);
        addAndMakeVisible (up);

        styleButton (down);
        styleButton (up);

        down.onClick = [this, i]
        {
            auto& s = interval[(size_t) i];
            s.setValue (s.getValue() - 1.0);
        };

        up.onClick = [this, i]
        {
            auto& s = interval[(size_t) i];
            s.setValue (s.getValue() + 1.0);
        };

        label (voiceLabels[(size_t) i],
               "V" + juce::String (i + 1), 9.0f);

        label (intervalLabels[(size_t) i],
               intervalNames[i], 7.0f);

        label (noteLabels[(size_t) i], "--", 9.0f, true);
    }

    // Input and harmony controls.
    setupSlider (gate, "gate", "GATE");
    setupSlider (response, "response", "CHORD RESP");

    // Tone and output controls.
    setupSlider (pitchLock, "pitchLock", "PITCH LOCK");
    setupSlider (glide, "glide", "GLIDE");
    setupSlider (humanize, "humanize", "HUMANIZE");
    setupSlider (formant, "formant", "FORMANT");
    setupSlider (consonants, "consonants", "CONSONANTS");
    setupSlider (dry, "dry", "DRY");
    setupSlider (harmony, "harmony", "HARMONY");

    // Per-voice controls.
    for (int i = 0; i < 4; ++i)
    {
        setupSlider (interval[(size_t) i],
                     "interval" + juce::String (i + 1),
                     "INTERVAL", false);

        setupSlider (level[(size_t) i],
                     "level" + juce::String (i + 1),
                     "LEVEL");

        setupSlider (pan[(size_t) i],
                     "pan" + juce::String (i + 1),
                     "PAN");
    }

    // Connect every slider to its APVTS parameter.
    int n = 0;

    for (auto pair : {
        std::pair<juce::Slider*, const char*> { &gate, "gate" },
        { &response, "response" },
        { &pitchLock, "pitchLock" },
        { &glide, "glide" },
        { &humanize, "humanize" },
        { &formant, "formant" },
        { &consonants, "consonants" },
        { &dry, "dry" },
        { &harmony, "harmony" },

        { &interval[0], "interval1" },
        { &level[0], "level1" },
        { &pan[0], "pan1" },

        { &interval[1], "interval2" },
        { &level[1], "level2" },
        { &pan[1], "pan2" },

        { &interval[2], "interval3" },
        { &level[2], "level3" },
        { &pan[2], "pan3" },

        { &interval[3], "interval4" },
        { &level[3], "level4" },
        { &pan[3], "pan4" }
    })
    {
        sliderAttachments[(size_t) n++] =
            std::make_unique<SliderAttachment> (
                processor.parameters, pair.second, *pair.first);
    }

    // Bypass.
    bypass.setClickingTogglesState (true);

    bypass.onClick = [this]
    {
        if (auto* param = processor.parameters.getParameter ("bypass"))
        {
            param->setValueNotifyingHost (
                bypass.getToggleState() ? 1.0f : 0.0f);
        }
    };

    // Reset all parameters to their defined defaults.
    reset.onClick = [this]
    {
        resetAllParameters();
    };

    // Copy the complete APVTS state as XML.
    copy.onClick = [this]
    {
        juce::SystemClipboard::copyTextToClipboard (
            processor.parameters.copyState().toXmlString());
    };

    // Paste only valid parameter-state XML.
    paste.onClick = [this]
    {
        auto xml = juce::parseXML (
            juce::SystemClipboard::getTextFromClipboard());

        if (xml != nullptr
            && xml->hasTagName (processor.parameters.state.getType()))
        {
            processor.parameters.replaceState (
                juce::ValueTree::fromXml (*xml));
        }
    };

    // Restore the default editor dimensions while keeping it centred.
    resetWindow.onClick = [this]
    {
        const auto centre = getBounds().getCentre();
        setSize (950, 410);
        setCentrePosition (centre);
    };
}

void RrHarmonizerAudioProcessorEditor::styleButton (
    juce::TextButton& b, bool isSelected)
{
    b.setColour (juce::TextButton::buttonColourId,
                 isSelected ? selected : button);

    b.setColour (juce::TextButton::buttonOnColourId, selected);
    b.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    b.setColour (juce::TextButton::textColourOnId, juce::Colours::white);

    b.setConnectedEdges (
        juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
}

void RrHarmonizerAudioProcessorEditor::setupSlider (
    juce::Slider& s, const juce::String& id,
    const juce::String& name, bool rotary)
{
    addAndMakeVisible (s);
    s.setName (id);

    s.setSliderStyle (
        rotary ? juce::Slider::RotaryHorizontalVerticalDrag
               : juce::Slider::LinearHorizontal);

    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 48, 13);

    s.setColour (juce::Slider::rotarySliderFillColourId, lime);
    s.setColour (juce::Slider::thumbColourId, lime);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, panelDark);
    s.setColour (juce::Slider::trackColourId, panelDark);
    s.setColour (juce::Slider::textBoxTextColourId, text);
    s.setColour (juce::Slider::textBoxBackgroundColourId, panel);

    s.setTooltip (name);
}

void RrHarmonizerAudioProcessorEditor::setupCombo (
    juce::ComboBox& c, const juce::String& id,
    const juce::StringArray& options)
{
    addAndMakeVisible (c);
    c.setName (id);

    for (int i = 0; i < options.size(); ++i)
        c.addItem (options[i], i + 1);

    c.setColour (juce::ComboBox::backgroundColourId,
                 juce::Colour (0xff555963));

    c.setColour (juce::ComboBox::textColourId, juce::Colours::white);

    c.setColour (juce::ComboBox::outlineColourId,
                 juce::Colour (0xff202025));
}

void RrHarmonizerAudioProcessorEditor::resetAllParameters()
{
    const juce::StringArray ids {
        "mode", "key", "scale", "source", "voices", "latency",
        "gate", "response", "pitchLock", "glide", "humanize",
        "formant", "consonants", "dry", "harmony", "bypass",
        "interval1", "level1", "pan1",
        "interval2", "level2", "pan2",
        "interval3", "level3", "pan3",
        "interval4", "level4", "pan4"
    };

    for (const auto& id : ids)
    {
        if (auto* param = processor.parameters.getParameter (id))
            param->setValueNotifyingHost (param->getDefaultValue());
    }

    bypass.setToggleState (false, juce::dontSendNotification);
}

void RrHarmonizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    // Header bars.
    g.setColour (header);
    g.fillRect (0, 0, getWidth(), 38);

    g.setColour (juce::Colour (0xff27272c));
    g.fillRect (0, 38, getWidth(), 38);

    g.setColour (lime);
    g.fillRect (10, 48, 3, 18);
    g.fillRect (10, 75, getWidth() - 20, 1);

    // Main panels.
    g.setColour (panel);
    g.fillRoundedRectangle (54.0f, 80.0f, 137.0f, 235.0f, 7.0f);
    g.fillRoundedRectangle (195.0f, 80.0f, 303.0f, 235.0f, 7.0f);
    g.fillRoundedRectangle (501.0f, 80.0f, 329.0f, 235.0f, 7.0f);
    g.fillRoundedRectangle (834.0f, 80.0f, 58.0f, 235.0f, 7.0f);

    g.fillRoundedRectangle (54.0f, 318.0f, 294.0f, 90.0f, 7.0f);
    g.fillRoundedRectangle (352.0f, 318.0f, 116.0f, 90.0f, 7.0f);

    // Input pitch display.
    g.setColour (panelDark);
    g.fillRoundedRectangle (62.0f, 98.0f, 122.0f, 56.0f, 5.0f);

    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (8.0f)));
    g.drawText ("no pitch", 65, 132, 116, 12,
                juce::Justification::centred);

    g.setColour (juce::Colour (0xff4a4a50));
    g.fillRoundedRectangle (71.0f, 147.0f, 104.0f, 6.0f, 3.0f);

    g.setColour (lime);
    g.fillRoundedRectangle (71.0f, 147.0f, 1.0f, 6.0f, 3.0f);

    // Harmony note monitor.
    g.setColour (panelDark);
    g.fillRoundedRectangle (202.0f, 162.0f, 202.0f, 46.0f, 4.0f);

    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (7.5f)));
    g.drawText ("C  C#  D  D#  E  F  F#  G  G#  A  A#  B",
                207, 194, 192, 10, juce::Justification::centred);

    g.setColour (text);
    g.setFont (juce::Font (juce::FontOptions (8.0f)));
    g.drawText ("Key: C# major  (auto pitch detection)",
                202, 146, 282, 12, juce::Justification::left);

    g.drawText ("CHORD SOURCE", 202, 218, 100, 12,
                juce::Justification::left);

    // Voice table headings and separators.
    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (8.0f).withStyle ("Bold")));

    g.drawText ("COUNT", 510, 103, 40, 10, juce::Justification::left);
    g.drawText ("NOTE", 672, 96, 40, 12, juce::Justification::centred);
    g.drawText ("LEVEL", 719, 96, 48, 12, juce::Justification::centred);
    g.drawText ("PAN", 770, 96, 42, 12, juce::Justification::centred);

    g.setColour (juce::Colour (0xff3a3a40));

    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine (132 + i * 43, 510.0f, 820.0f);

    // Output meter shell and decibel markings.
    g.setColour (panelDark);
    g.fillRoundedRectangle (855.0f, 103.0f, 25.0f, 208.0f, 3.0f);

    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (8.0f)));

    const juce::StringArray meterTicks {
        "0", "-6", "-12", "-20", "-40"
    };

    for (int i = 0; i < meterTicks.size(); ++i)
    {
        g.drawText (meterTicks[i], 836, 109 + i * 43, 18, 10,
                    juce::Justification::right);
    }

    g.setColour (juce::Colour (0xff34343a));
    g.drawRect (861, 109, 13, 198, 1);

    // Control captions.
    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (7.0f)));

    g.drawText ("GATE", 79, 293, 76, 10,
                juce::Justification::centred);

    g.drawText ("CHORD RESP", 414, 232, 72, 10,
                juce::Justification::centred);

    g.drawText ("PITCH LOCK", 62, 392, 48, 9,
                juce::Justification::centred);

    g.drawText ("GLIDE", 120, 392, 48, 9,
                juce::Justification::centred);

    g.drawText ("HUMANIZE", 178, 392, 48, 9,
                juce::Justification::centred);

    g.drawText ("FORMANT", 236, 392, 48, 9,
                juce::Justification::centred);

    g.drawText ("CONSONANTS", 294, 392, 48, 9,
                juce::Justification::centred);

    g.drawText ("DRY", 359, 392, 48, 9,
                juce::Justification::centred);

    g.drawText ("HARMONY", 414, 392, 48, 9,
                juce::Justification::centred);
}

void RrHarmonizerAudioProcessorEditor::resized()
{
    title.setBounds (32, 5, 150, 25);
    pitchTab.setBounds (120, 43, 50, 24);

    undo.setBounds (508, 47, 40, 23);
    redo.setBounds (552, 47, 40, 23);
    copy.setBounds (597, 47, 43, 23);
    paste.setBounds (644, 47, 45, 23);
    reset.setBounds (693, 47, 45, 23);
    bypass.setBounds (743, 47, 72, 23);
    resetWindow.setBounds (819, 47, 107, 23);

    sectionInput.setBounds (64, 83, 70, 12);
    sectionHarmony.setBounds (203, 83, 100, 12);
    sectionVoices.setBounds (509, 83, 80, 12);
    sectionTone.setBounds (64, 321, 60, 12);
    sectionOutput.setBounds (362, 321, 70, 12);

    pitchReadout.setBounds (212, 164, 180, 38);
    statusReadout.setBounds (202, 147, 260, 12);
    noPitch.setBounds (70, 128, 106, 12);

    modeButtons[0].setBounds (202, 99, 92, 20);
    modeButtons[1].setBounds (299, 99, 92, 20);
    modeButtons[2].setBounds (396, 99, 92, 20);

    manualButton.setBounds (202, 124, 58, 20);
    autoButton.setBounds (263, 124, 39, 20);

    key.setBounds (307, 124, 54, 20);
    scale.setBounds (365, 124, 123, 20);

    source.setBounds (202, 233, 168, 20);
    sourcePlus.setBounds (374, 233, 30, 20);

    for (int i = 0; i < 3; ++i)
        latencyButtons[(size_t) i].setBounds (62 + i * 43, 165, 38, 19);

    gate.setBounds (79, 202, 76, 90);
    response.setBounds (421, 156, 58, 77);

    for (int i = 0; i < 4; ++i)
    {
        countButtons[(size_t) i].setBounds (572 + i * 31, 99, 26, 19);

        const int y = 132 + i * 43;

        voiceLabels[(size_t) i].setBounds (510, y + 7, 22, 14);
        intervalDown[(size_t) i].setBounds (530, y, 20, 18);
        intervalUp[(size_t) i].setBounds (553, y, 20, 18);
        intervalLabels[(size_t) i].setBounds (530, y + 19, 68, 12);
        interval[(size_t) i].setBounds (598, y + 1, 50, 27);

        noteLabels[(size_t) i].setBounds (672, y + 8, 40, 14);
        level[(size_t) i].setBounds (716, y - 1, 48, 39);
        pan[(size_t) i].setBounds (766, y - 1, 48, 39);
    }

    pitchLock.setBounds (62, 332, 48, 57);
    glide.setBounds (120, 332, 48, 57);
    humanize.setBounds (178, 332, 48, 57);
    formant.setBounds (236, 332, 48, 57);
    consonants.setBounds (294, 332, 48, 57);

    dry.setBounds (359, 332, 48, 57);
    harmony.setBounds (414, 332, 48, 57);
}
