```cpp
#include "PluginEditor.h"

namespace
{
    const juce::Colour backgroundColour  { 18, 21, 29 };
    const juce::Colour panelColour       { 28, 33, 44 };
    const juce::Colour panelLightColour  { 38, 45, 59 };
    const juce::Colour accentColour      { 64, 211, 190 };
    const juce::Colour textColour        { 239, 243, 250 };
    const juce::Colour mutedColour       { 151, 163, 181 };
    const juce::Colour dangerColour      { 235, 105, 105 };

    juce::String midiNoteName (int midiNote)
    {
        static const char* names[] =
        {
            "C", "C#", "D", "D#", "E", "F",
            "F#", "G", "G#", "A", "A#", "B"
        };

        const int note = ((midiNote % 12) + 12) % 12;
        const int octave = midiNote / 12 - 1;

        return juce::String (names[note]) + juce::String (octave);
    }
}

//==============================================================================
RrHarmonizerAudioProcessorEditor::RrHarmonizerAudioProcessorEditor
    (RrHarmonizerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1000, 620);
    setResizable (true, true);
    setResizeLimits (820, 540, 1500, 950);

    // Header
    title.setText ("RR HARMONIZER", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (25.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, textColour);
    addAndMakeVisible (title);

    pitchTab.setText ("VOCAL HARMONY ENGINE", juce::dontSendNotification);
    pitchTab.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    pitchTab.setColour (juce::Label::textColourId, accentColour);
    addAndMakeVisible (pitchTab);

    // Section headings
    const auto setupHeading = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        label.setColour (juce::Label::textColourId, accentColour);
        addAndMakeVisible (label);
    };

    setupHeading (sectionInput,   "INPUT / KEY");
    setupHeading (sectionHarmony, "HARMONY ENGINE");
    setupHeading (sectionVoices,  "VOICE MIX");
    setupHeading (sectionTone,    "VOICE CHARACTER");
    setupHeading (sectionOutput,  "OUTPUT");

    // Readouts
    pitchReadout.setText ("PITCH: --", juce::dontSendNotification);
    pitchReadout.setFont (juce::Font (juce::FontOptions (17.0f, juce::Font::bold)));
    pitchReadout.setColour (juce::Label::textColourId, textColour);
    pitchReadout.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (pitchReadout);

    statusReadout.setText ("READY", juce::dontSendNotification);
    statusReadout.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    statusReadout.setColour (juce::Label::textColourId, accentColour);
    addAndMakeVisible (statusReadout);

    noteHeading.setText ("INTERVAL", juce::dontSendNotification);
    levelHeading.setText ("LEVEL", juce::dontSendNotification);
    panHeading.setText ("PAN", juce::dontSendNotification);
    outHeading.setText ("DRY / HARMONY", juce::dontSendNotification);

    for (auto* label : { &noteHeading, &levelHeading, &panHeading, &outHeading })
    {
        label->setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        label->setColour (juce::Label::textColourId, mutedColour);
        addAndMakeVisible (*label);
    }

    noPitch.setText ("Sing into your input to detect pitch", juce::dontSendNotification);
    noPitch.setFont (juce::Font (juce::FontOptions (11.0f)));
    noPitch.setColour (juce::Label::textColourId, mutedColour);
    addAndMakeVisible (noPitch);

    chordSourceLabel.setText ("HARMONY MODE", juce::dontSendNotification);
    chordSourceLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    chordSourceLabel.setColour (juce::Label::textColourId, mutedColour);
    addAndMakeVisible (chordSourceLabel);

    // Comboboxes
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

    // Global controls
    setupSlider (gate,       "gate",       "Gate");
    setupSlider (response,   "response",   "Response");
    setupSlider (pitchLock,  "pitchLock",  "Pitch Lock");
    setupSlider (glide,      "glide",      "Glide");
    setupSlider (humanize,   "humanize",   "Humanize");
    setupSlider (formant,    "formant",    "Formant");
    setupSlider (consonants, "consonants", "Consonants");
    setupSlider (dry,        "dry",        "Dry Level");
    setupSlider (harmony,    "harmony",    "Harmony Level");

    int attachmentIndex = 0;

    sliderAttachments[attachmentIndex++] =
        std::make_unique<SliderAttachment> (processor.parameters, "gate", gate);
    sliderAttachments[attachmentIndex++] =
        std::make_unique<SliderAttachment> (processor.parameters, "response", response);
    sliderAttachments[attachmentIndex++] =
        std::make_unique<SliderAttachment> (processor.parameters, "pitchLock", pitchLock);
    sliderAttachments[attachmentIndex++] =
        std::make_unique<SliderAttachment> (processor.parameters, "glide", glide);
    sliderAttachments[attachmentIndex++] =
        std::make_unique<SliderAttachment> (processor.parameters, "humanize", humanize);
    sliderAttachments[attachmentIndex++] =
        std::make_unique<SliderAttachment> (processor.parameters, "formant", formant);
    sliderAttachments[attachmentIndex++] =
        std::make_unique<SliderAttachment> (processor.parameters, "consonants", consonants);
    sliderAttachments[attachmentIndex++] =
        std::make_unique<SliderAttachment> (processor.parameters, "dry", dry);
    sliderAttachments[attachmentIndex++] =
        std::make_unique<SliderAttachment> (processor.parameters, "harmony", harmony);

    // Voice controls
    for (int i = 0; i < 4; ++i)
    {
        const int voiceNumber = i + 1;

        voiceLabels[(size_t) i].setText ("VOICE " + juce::String (voiceNumber),
                                         juce::dontSendNotification);
        voiceLabels[(size_t) i].setFont (
            juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        voiceLabels[(size_t) i].setColour (
            juce::Label::textColourId, textColour);
        addAndMakeVisible (voiceLabels[(size_t) i]);

        intervalLabels[(size_t) i].setText ("0 st", juce::dontSendNotification);
        intervalLabels[(size_t) i].setFont (juce::Font (juce::FontOptions (11.0f)));
        intervalLabels[(size_t) i].setColour (
            juce::Label::textColourId, textColour);
        intervalLabels[(size_t) i].setJustificationType (
            juce::Justification::centred);
        addAndMakeVisible (intervalLabels[(size_t) i]);

        noteLabels[(size_t) i].setText ("VOICE " + juce::String (voiceNumber),
                                        juce::dontSendNotification);
        noteLabels[(size_t) i].setFont (juce::Font (juce::FontOptions (10.0f)));
        noteLabels[(size_t) i].setColour (
            juce::Label::textColourId, mutedColour);
        noteLabels[(size_t) i].setJustificationType (
            juce::Justification::centred);
        addAndMakeVisible (noteLabels[(size_t) i]);

        setupSlider (interval[(size_t) i],
                     "interval" + juce::String (voiceNumber), "Interval", false);
        setupSlider (level[(size_t) i],
                     "level" + juce::String (voiceNumber), "Level", true);
        setupSlider (pan[(size_t) i],
                     "pan" + juce::String (voiceNumber), "Pan", false);

        sliderAttachments[attachmentIndex++] =
            std::make_unique<SliderAttachment> (
                processor.parameters,
                "interval" + juce::String (voiceNumber),
                interval[(size_t) i]);

        sliderAttachments[attachmentIndex++] =
            std::make_unique<SliderAttachment> (
                processor.parameters,
                "level" + juce::String (voiceNumber),
                level[(size_t) i]);

        sliderAttachments[attachmentIndex++] =
            std::make_unique<SliderAttachment> (
                processor.parameters,
                "pan" + juce::String (voiceNumber),
                pan[(size_t) i]);

        interval[(size_t) i].onValueChange = [this, i]
        {
            intervalLabels[(size_t) i].setText (
                juce::String ((int) interval[(size_t) i].getValue()) + " st",
                juce::dontSendNotification);
        };

        intervalDown[(size_t) i].setButtonText ("−");
        intervalUp[(size_t) i].setButtonText ("+");
        styleButton (intervalDown[(size_t) i]);
        styleButton (intervalUp[(size_t) i]);

        intervalDown[(size_t) i].onClick = [this, i]
        {
            saveUndoState();
            interval[(size_t) i].setValue (
                interval[(size_t) i].getValue() - 1.0,
                juce::sendNotificationSync);
        };

        intervalUp[(size_t) i].onClick = [this, i]
        {
            saveUndoState();
            interval[(size_t) i].setValue (
                interval[(size_t) i].getValue() + 1.0,
                juce::sendNotificationSync);
        };

        addAndMakeVisible (intervalDown[(size_t) i]);
        addAndMakeVisible (intervalUp[(size_t) i]);
    }

    // Mode selection
    modeButtons[0].setButtonText ("SCALE");
    modeButtons[1].setButtonText ("CHORD");
    modeButtons[2].setButtonText ("FIXED");

    for (int i = 0; i < 3; ++i)
    {
        styleButton (modeButtons[(size_t) i]);

        modeButtons[(size_t) i].onClick = [this, i]
        {
            saveUndoState();
            if (auto* parameter = processor.parameters.getParameter ("mode"))
                parameter->setValueNotifyingHost (
                    (float) i / 2.0f);

            updateModeButtons();
        };

        addAndMakeVisible (modeButtons[(size_t) i]);
    }

    manualButton.setButtonText ("MANUAL");
    autoButton.setButtonText ("AUTO");
    sourcePlus.setButtonText ("+");

    for (auto* button : { &manualButton, &autoButton, &sourcePlus })
    {
        styleButton (*button);
        addAndMakeVisible (*button);
    }

    manualButton.onClick = [this]
    {
        saveUndoState();
        source.setSelectedId (2, juce::sendNotificationSync);
        updateSourceButtons();
    };

    autoButton.onClick = [this]
    {
        saveUndoState();
        source.setSelectedId (1, juce::sendNotificationSync);
        updateSourceButtons();
    };

    sourcePlus.onClick = [this]
    {
        saveUndoState();
        const int next = source.getSelectedId() == 1 ? 2 : 1;
        source.setSelectedId (next, juce::sendNotificationSync);
        updateSourceButtons();
    };

    // Latency buttons
    latencyButtons[0].setButtonText ("LOW");
    latencyButtons[1].setButtonText ("MID");
    latencyButtons[2].setButtonText ("HIGH");

    for (int i = 0; i < 3; ++i)
    {
        styleButton (latencyButtons[(size_t) i]);

        latencyButtons[(size_t) i].onClick = [this, i]
        {
            saveUndoState();
            if (auto* parameter = processor.parameters.getParameter ("latency"))
                parameter->setValueNotifyingHost ((float) i / 2.0f);

            updateLatencyButtons();
        };

        addAndMakeVisible (latencyButtons[(size_t) i]);
    }

    // Voice count buttons
    for (int i = 0; i < 4; ++i)
    {
        countButtons[(size_t) i].setButtonText (juce::String (i + 1));
        styleButton (countButtons[(size_t) i]);

        countButtons[(size_t) i].onClick = [this, i]
        {
            saveUndoState();
            if (auto* parameter = processor.parameters.getParameter ("voices"))
                parameter->setValueNotifyingHost ((float) i / 3.0f);

            updateVoiceButtons();
        };

        addAndMakeVisible (countButtons[(size_t) i]);
    }

    // Utility buttons
    for (auto* button : { &undo, &redo, &copy, &paste, &reset,
                          &bypass, &resetWindow })
    {
        styleButton (*button);
        addAndMakeVisible (*button);
    }

    undo.onClick = [this] { performUndo(); };
    redo.onClick = [this] { performRedo(); };

    copy.onClick = [this]
    {
        auto state = processor.parameters.copyState();
        std::unique_ptr<juce::XmlElement> xml (state.createXml());

        if (xml != nullptr)
            juce::SystemClipboard::copyTextToClipboard (xml->toString());
    };

    paste.onClick = [this]
    {
        const auto text = juce::SystemClipboard::getTextFromClipboard();
        std::unique_ptr<juce::XmlElement> xml (
            juce::XmlDocument::parse (text));

        if (xml != nullptr
            && xml->hasTagName (processor.parameters.state.getType().toString()))
        {
            saveUndoState();
            auto tree = juce::ValueTree::fromXml (*xml);

            if (tree.isValid())
                processor.parameters.replaceState (tree);

            updateModeButtons();
            updateVoiceButtons();
            updateLatencyButtons();
            updateSourceButtons();
        }
    };

    reset.onClick = [this] { resetAllParameters(); };

    bypass.onClick = [this]
    {
        saveUndoState();

        if (auto* parameter = processor.parameters.getParameter ("bypass"))
        {
            const bool currentlyBypassed =
                parameter->getValue() >= 0.5f;

            parameter->setValueNotifyingHost (
                currentlyBypassed ? 0.0f : 1.0f);
        }

        const bool isBypassed =
            processor.parameters.getRawParameterValue ("bypass")->load() >= 0.5f;

        bypass.setButtonText (isBypassed ? "BYPASSED" : "BYPASS");
        bypass.setColour (
            juce::TextButton::buttonColourId,
            isBypassed ? dangerColour : panelLightColour);
    };

    resetWindow.onClick = [this]
    {
        setSize (1000, 620);
    };

    // Slider changes are captured before the user starts dragging.
    auto attachUndoOnDrag = [this] (juce::Slider& slider)
    {
        slider.onDragStart = [this] { saveUndoState(); };
    };

    attachUndoOnDrag (gate);
    attachUndoOnDrag (response);
    attachUndoOnDrag (pitchLock);
    attachUndoOnDrag (glide);
    attachUndoOnDrag (humanize);
    attachUndoOnDrag (formant);
    attachUndoOnDrag (consonants);
    attachUndoOnDrag (dry);
    attachUndoOnDrag (harmony);

    for (int i = 0; i < 4; ++i)
    {
        attachUndoOnDrag (interval[(size_t) i]);
        attachUndoOnDrag (level[(size_t) i]);
        attachUndoOnDrag (pan[(size_t) i]);
    }

    updateModeButtons();
    updateVoiceButtons();
    updateLatencyButtons();
    updateSourceButtons();
    updatePitchReadout();

    // Store the initial state so the undo system starts with a valid snapshot.
    undoHistory.clear();
    redoHistory.clear();
}

//==============================================================================
void RrHarmonizerAudioProcessorEditor::setupSlider
    (juce::Slider& slider, const juce::String& parameterID,
     const juce::String& label, bool rotary)
{
    slider.setName (label);
    slider.setTooltip (label);
    slider.setColour (juce::Slider::textBoxTextColourId, textColour);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panelColour);
    slider.setColour (juce::Slider::textBoxOutlineColourId, panelLightColour);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, panelLightColour);
    slider.setColour (juce::Slider::thumbColourId, accentColour);
    slider.setColour (juce::Slider::trackColourId, accentColour);
    slider.setColour (juce::Slider::backgroundColourId, panelLightColour);

    if (rotary)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 20);
    }
    else
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 20);
