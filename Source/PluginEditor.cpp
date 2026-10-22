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
    }

    addAndMakeVisible (slider);
}

void RrHarmonizerAudioProcessorEditor::setupCombo
    (juce::ComboBox& combo, const juce::String& parameterID,
     const juce::StringArray& items)
{
    combo.clear();

    for (int i = 0; i < items.size(); ++i)
        combo.addItem (items[i], i + 1);

    combo.setTooltip (parameterID);
    combo.setColour (juce::ComboBox::backgroundColourId, panelLightColour);
    combo.setColour (juce::ComboBox::textColourId, textColour);
    combo.setColour (juce::ComboBox::outlineColourId, mutedColour);
    combo.setColour (juce::ComboBox::arrowColourId, accentColour);
    addAndMakeVisible (combo);
}

void RrHarmonizerAudioProcessorEditor::styleButton
    (juce::TextButton& button, bool selected)
{
    button.setClickingTogglesState (false);
    button.setColour (
        juce::TextButton::buttonColourId,
        selected ? accentColour : panelLightColour);
    button.setColour (
        juce::TextButton::textColourOffId,
        selected ? backgroundColour : textColour);
    button.setColour (
        juce::TextButton::textColourOnId, backgroundColour);
    button.setColour (
        juce::TextButton::buttonOnColourId, accentColour);
    button.setConnectedEdges (juce::Button::ConnectedOnLeft
                              | juce::Button::ConnectedOnRight);
}

//==============================================================================
void RrHarmonizerAudioProcessorEditor::updateModeButtons()
{
    const int mode = juce::jlimit (
        0, 2,
        (int) std::round (
            processor.parameters.getRawParameterValue ("mode")->load() * 2.0f));

    for (int i = 0; i < 3; ++i)
        styleButton (modeButtons[(size_t) i], i == mode);
}

void RrHarmonizerAudioProcessorEditor::updateVoiceButtons()
{
    const int voices = juce::jlimit (
        1, 4,
        1 + (int) std::round (
            processor.parameters.getRawParameterValue ("voices")->load() * 3.0f));

    for (int i = 0; i < 4; ++i)
        styleButton (countButtons[(size_t) i], i + 1 == voices);
}

void RrHarmonizerAudioProcessorEditor::updateLatencyButtons()
{
    const int latency = juce::jlimit (
        0, 2,
        (int) std::round (
            processor.parameters.getRawParameterValue ("latency")->load() * 2.0f));

    for (int i = 0; i < 3; ++i)
        styleButton (latencyButtons[(size_t) i], i == latency);
}

void RrHarmonizerAudioProcessorEditor::updateSourceButtons()
{
    const bool isManual = source.getSelectedId() == 2;
    styleButton (manualButton, isManual);
    styleButton (autoButton, ! isManual);
}

void RrHarmonizerAudioProcessorEditor::updatePitchReadout()
{
    const int midiNote = processor.getDetectedMidi();

    if (midiNote >= 0 && midiNote <= 127)
    {
        pitchReadout.setText (
            "PITCH: " + midiNoteName (midiNote),
            juce::dontSendNotification);
        statusReadout.setText ("PITCH DETECTED", juce::dontSendNotification);
    }
    else
    {
        pitchReadout.setText ("PITCH: --", juce::dontSendNotification);
        statusReadout.setText ("WAITING FOR INPUT", juce::dontSendNotification);
    }
}

//==============================================================================
void RrHarmonizerAudioProcessorEditor::saveUndoState()
{
    if (restoringState)
        return;

    undoHistory.push_back (processor.parameters.copyState());
    if (undoHistory.size() > 64)
        undoHistory.erase (undoHistory.begin());

    redoHistory.clear();
}

void RrHarmonizerAudioProcessorEditor::performUndo()
{
    if (undoHistory.empty())
        return;

    restoringState = true;
    redoHistory.push_back (processor.parameters.copyState());

    const auto state = undoHistory.back();
    undoHistory.pop_back();

    processor.parameters.replaceState (state);
    restoringState = false;

    updateModeButtons();
    updateVoiceButtons();
    updateLatencyButtons();
    updateSourceButtons();
    updatePitchReadout();
}

void RrHarmonizerAudioProcessorEditor::performRedo()
{
    if (redoHistory.empty())
        return;

    restoringState = true;
    undoHistory.push_back (processor.parameters.copyState());

    const auto state = redoHistory.back();
    redoHistory.pop_back();

    processor.parameters.replaceState (state);
    restoringState = false;

    updateModeButtons();
    updateVoiceButtons();
    updateLatencyButtons();
    updateSourceButtons();
    updatePitchReadout();
}

void RrHarmonizerAudioProcessorEditor::resetAllParameters()
{
    saveUndoState();

    for (auto* parameter : processor.getParameters())
        parameter->setValueNotifyingHost (parameter->getDefaultValue());

    updateModeButtons();
    updateVoiceButtons();
    updateLatencyButtons();
    updateSourceButtons();
    updatePitchReadout();

    bypass.setButtonText ("BYPASS");
    bypass.setColour (
        juce::TextButton::buttonColourId, panelLightColour);
}

//==============================================================================
void RrHarmonizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour);

    const auto bounds = getLocalBounds().toFloat();

    // Header strip
    g.setColour (panelColour);
    g.fillRoundedRectangle (
        bounds.removeFromTop (72.0f).reduced (10.0f, 6.0f), 10.0f);

    // Main section panels
    const int margin = 10;
    const int top = 90;
    const int bottom = getHeight() - 12;
    const int contentHeight = bottom - top;

    const int inputWidth = juce::roundToInt (getWidth() * 0.25f);
    const int harmonyWidth = juce::roundToInt (getWidth() * 0.25f);
    const int voiceWidth = juce::roundToInt (getWidth() * 0.27f);
    const int toneWidth = juce::roundToInt (getWidth() * 0.12f);
    const int outputWidth = getWidth() - inputWidth - harmonyWidth
                          - voiceWidth - toneWidth - margin * 6;

    int x = margin;

    for (const int width : { inputWidth, harmonyWidth, voiceWidth,
                             toneWidth, juce::jmax (100, outputWidth) })
    {
        g.setColour (panelColour);
        g.fillRoundedRectangle (
            (float) x, (float) top, (float) width,
            (float) contentHeight, 10.0f);
        x += width + margin;
    }

    g.setColour (panelLightColour);
    g.drawRoundedRectangle (
        10.0f, 6.0f, (float) getWidth() - 20.0f, 60.0f, 10.0f, 1.0f);

    // Small signal meter, driven by the processor's output-level readout.
    const float levelValue = juce::jlimit (
        0.0f, 1.0f, processor.getOutputLevel());

    const int meterX = getWidth() - 128;
    const int meterY = 43;
    const int meterW = 100;
    const int meterH = 8;

    g.setColour (panelLightColour);
    g.fillRoundedRectangle (
        (float) meterX, (float) meterY,
        (float) meterW, (float) meterH, 4.0f);

    g.setColour (accentColour);
    g.fillRoundedRectangle (
        (float) meterX, (float) meterY,
        (float) meterW * levelValue, (float) meterH, 4.0f);
}

//==============================================================================
void RrHarmonizerAudioProcessorEditor::resized()
{
    const int width = getWidth();
    const int height = getHeight();
    const int margin = 10;
    const int headerY = 10;

    title.setBounds (22, headerY, 280, 34);
    pitchTab.setBounds (24, 43, 300, 20);
    pitchReadout.setBounds (width - 280, 12, 150, 28);
    statusReadout.setBounds (width - 280, 40, 160, 18);

    const int utilityY = 76;
    const int utilityH = 28;
    const int utilityW = 78;
    int utilityX = width - margin - utilityW;

    resetWindow.setBounds (utilityX, utilityY, utilityW, utilityH);
    utilityX -= utilityW + 5;
    reset.setBounds (utilityX, utilityY, utilityW, utilityH);
    utilityX -= utilityW + 5;
    paste.setBounds (utilityX, utilityY, utilityW, utilityH);
    utilityX -= utilityW + 5;
    copy.setBounds (utilityX, utilityY, utilityW, utilityH);
    utilityX -= utilityW + 5;
    redo.setBounds (utilityX, utilityY, utilityW, utilityH);
    utilityX -= utilityW + 5;
    undo.setBounds (utilityX, utilityY, utilityW, utilityH);
    utilityX -= utilityW + 5;
    bypass.setBounds (utilityX, utilityY, utilityW, utilityH);

    const int top = 116;
    const int contentHeight = height - top - margin;
    const int gap = 8;

    const int inputW = juce::roundToInt (width * 0.235f);
    const int harmonyW = juce::roundToInt (width * 0.245f);
    const int voiceW = juce::roundToInt (width * 0.265f);
    const int toneW = juce::roundToInt (width * 0.125f);
    const int outputW = width - inputW - harmonyW - voiceW - toneW
                      - margin * 2 - gap * 4;

    int x = margin;

    const int inputX = x;
    sectionInput.setBounds (inputX + 12, top + 10, inputW - 24, 22);
    key.setBounds (inputX + 12, top + 42, inputW - 24, 30);
    scale.setBounds (inputX + 12, top + 78, inputW - 24, 30);
    chordSourceLabel.setBounds (inputX + 12, top + 116, inputW - 24, 18);
    source.setBounds (inputX + 12, top + 138, inputW - 24, 28);
    autoButton.setBounds (inputX + 12, top + 174, (inputW - 30) / 2, 28);
    manualButton.setBounds (
        inputX + 18 + (inputW - 30) / 2,
        top + 174, (inputW - 30) / 2, 28);
    sourcePlus.setBounds (inputX + inputW - 40, top + 10, 26, 22);
    noPitch.setBounds (inputX + 12, top + 216, inputW - 24, 28);
    x += inputW + gap;

    const int harmonyX = x;
    sectionHarmony.setBounds (harmonyX + 12, top + 10, harmonyW - 24, 22);

    const int modeButtonW = (harmonyW - 32) / 3;
    for (int i = 0; i < 3; ++i)
        modeButtons[(size_t) i].setBounds (
            harmonyX + 12 + i * (modeButtonW + 4),
            top + 40, modeButtonW, 28);

    sectionTone.setBounds (harmonyX + 12, top + 80, harmonyW - 24, 20);

    const int controlW = (harmonyW - 30) / 3;
    const int controlH = juce::jmax (90, (contentHeight - 130) / 2);

    gate.setBounds (harmonyX + 10, top + 104, controlW, controlH);
    response.setBounds (harmonyX + 15 + controlW, top + 104, controlW, controlH);
    pitchLock.setBounds (harmonyX + 20 + controlW * 2, top + 104,
                         controlW, controlH);

    glide.setBounds (harmonyX + 10, top + 104 + controlH, controlW, controlH);
    humanize.setBounds (harmonyX + 15 + controlW, top + 104 + controlH,
                        controlW, controlH);
    formant.setBounds (harmonyX + 20 + controlW * 2, top + 104 + controlH,
                       controlW, controlH);

    x += harmonyW + gap;

    const int voiceX = x;
    sectionVoices.setBounds (voiceX + 12, top + 10, voiceW - 24, 22);

    const int countW = (voiceW - 32) / 4;
    for (int i = 0; i < 4; ++i)
        countButtons[(size_t) i].setBounds (
            voiceX + 12 + i * (countW + 3), top + 40, countW, 27);

    noteHeading.setBounds (voiceX + 12, top + 76, voiceW - 24, 18);

    const int voiceGap = 5;
    const int oneVoiceW = (voiceW - 24 - voiceGap * 3) / 4;
    const int voiceTop = top + 96;
    const int voiceBottom = top + contentHeight - 8;
    const int availableVoiceH = voiceBottom - voiceTop;

    for (int i = 0; i < 4; ++i)
    {
        const int vx = voiceX + 12 + i * (oneVoiceW + voiceGap);

        voiceLabels[(size_t) i].setBounds (vx, voiceTop, oneVoiceW, 20);
        intervalLabels[(size_t) i].setBounds (vx, voiceTop + 24, oneVoiceW, 20);

        intervalDown[(size_t) i].setBounds (vx, voiceTop + 48, 24, 23);
        intervalUp[(size_t) i].setBounds (
            vx + oneVoiceW - 24, voiceTop + 48, 24, 23);

        interval[(size_t) i].setBounds (
            vx, voiceTop + 76, oneVoiceW, 38);

        level[(size_t) i].setBounds (
            vx, voiceTop + 116, oneVoiceW, juce::jmax (70, availableVoiceH / 3));

        pan[(size_t) i].setBounds (
            vx, voiceTop + 116 + juce::jmax (70, availableVoiceH / 3),
            oneVoiceW, 42);

        noteLabels[(size_t) i].setBounds (
            vx, voiceBottom - 20, oneVoiceW, 18);
    }

    x += voiceW + gap;

    const int toneX = x;
    sectionTone.setBounds (toneX + 10, top + 10, toneW - 20, 22);
    consonants.setBounds (toneX + 5, top + 44, toneW - 10,
                          juce::jmax (100, contentHeight / 3));
    dry.setBounds (toneX + 5, top + 44 + juce::jmax (100, contentHeight / 3),
                   toneW - 10, juce::jmax (100, contentHeight / 3));
    harmony.setBounds (
        toneX + 5, top + 44 + juce::jmax (100, contentHeight / 3) * 2,
        toneW - 10, juce::jmax (100, contentHeight / 3));

    x += toneW + gap;

    const int outputX = x;
    sectionOutput.setBounds (outputX + 8, top + 10, outputW - 16, 22);
    outHeading.setBounds (outputX + 8, top + 40, outputW - 16, 20);

    latencyButtons[0].setBounds (outputX + 8, top + 72,
                                 outputW - 16, 26);
    latencyButtons[1].setBounds (outputX + 8, top + 103,
                                 outputW - 16, 26);
    latencyButtons[2].setBounds (outputX + 8, top + 134,
                                 outputW - 16, 26);
}
```
