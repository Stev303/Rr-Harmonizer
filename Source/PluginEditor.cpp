#include "PluginEditor.h"

namespace
{
    const juce::Colour bg       { 17, 20, 28 };
    const juce::Colour panel    { 28, 33, 44 };
    const juce::Colour buttonBg { 43, 50, 64 };
    const juce::Colour accent   { 60, 220, 190 };
    const juce::Colour white    { 240, 244, 250 };
    const juce::Colour muted    { 150, 163, 180 };

    juce::String noteName (int midi)
    {
        static const char* notes[] {
            "C", "C#", "D", "D#", "E", "F",
            "F#", "G", "G#", "A", "A#", "B"
        };

        return juce::String (notes[((midi % 12) + 12) % 12])
             + juce::String (midi / 12 - 1);
    }
}

RrHarmonizerAudioProcessorEditor::RrHarmonizerAudioProcessorEditor
    (RrHarmonizerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1100, 700);
    setResizable (true, true);
    setResizeLimits (1000, 650, 1500, 950);

    auto heading = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        l.setColour (juce::Label::textColourId, accent);
        addAndMakeVisible (l);
    };

    title.setText ("RR HARMONIZER", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, white);
    addAndMakeVisible (title);

    pitchTab.setText ("VOCAL HARMONY ENGINE", juce::dontSendNotification);
    pitchTab.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    pitchTab.setColour (juce::Label::textColourId, accent);
    addAndMakeVisible (pitchTab);

    heading (sectionInput, "INPUT / KEY");
    heading (sectionHarmony, "HARMONY ENGINE");
    heading (sectionVoices, "VOICE MIX");
    heading (sectionOutput, "OUTPUT");

    pitchReadout.setText ("PITCH: --", juce::dontSendNotification);
    pitchReadout.setFont (juce::Font (juce::FontOptions (17.0f, juce::Font::bold)));
    pitchReadout.setColour (juce::Label::textColourId, white);
    addAndMakeVisible (pitchReadout);

    statusReadout.setText ("READY", juce::dontSendNotification);
    statusReadout.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    statusReadout.setColour (juce::Label::textColourId, accent);
    addAndMakeVisible (statusReadout);

    noteHeading.setText ("INTERVAL", juce::dontSendNotification);
    levelHeading.setText ("LEVEL", juce::dontSendNotification);
    panHeading.setText ("PAN", juce::dontSendNotification);
    outHeading.setText ("OUTPUT SETTINGS", juce::dontSendNotification);

    for (auto* l : { &noteHeading, &levelHeading, &panHeading, &outHeading })
    {
        l->setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        l->setColour (juce::Label::textColourId, muted);
        addAndMakeVisible (*l);
    }

    noPitch.setText ("Sing into your input to detect pitch",
                     juce::dontSendNotification);
    noPitch.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (noPitch);

    chordSourceLabel.setText ("CONTROL SOURCE", juce::dontSendNotification);
    chordSourceLabel.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (chordSourceLabel);

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

    setupSlider (gate, "gate", "Gate");
    setupSlider (response, "response", "Response");
    setupSlider (pitchLock, "pitchLock", "Pitch Lock");
    setupSlider (glide, "glide", "Glide");
    setupSlider (humanize, "humanize", "Humanize");
    setupSlider (formant, "formant", "Formant");
    setupSlider (consonants, "consonants", "Consonants");
    setupSlider (dry, "dry", "Dry Level");
    setupSlider (harmony, "harmony", "Harmony Level");

    int ai = 0;

    const juce::String globalIDs[] {
        "gate", "response", "pitchLock", "glide", "humanize",
        "formant", "consonants", "dry", "harmony"
    };

    juce::Slider* globalSliders[] {
        &gate, &response, &pitchLock, &glide, &humanize,
        &formant, &consonants, &dry, &harmony
    };

    for (int i = 0; i < 9; ++i)
        sliderAttachments[(size_t) ai++] =
            std::make_unique<SliderAttachment> (
                processor.parameters, globalIDs[i], *globalSliders[i]);

    for (int i = 0; i < 4; ++i)
    {
        const auto n = juce::String (i + 1);

        voiceLabels[(size_t) i].setText ("VOICE " + n,
                                         juce::dontSendNotification);
        voiceLabels[(size_t) i].setFont (
            juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        voiceLabels[(size_t) i].setColour (
            juce::Label::textColourId, white);
        voiceLabels[(size_t) i].setJustificationType (
            juce::Justification::centred);
        addAndMakeVisible (voiceLabels[(size_t) i]);

        intervalLabels[(size_t) i].setText ("0 st",
                                            juce::dontSendNotification);
        intervalLabels[(size_t) i].setColour (
            juce::Label::textColourId, accent);
        intervalLabels[(size_t) i].setJustificationType (
            juce::Justification::centred);
        addAndMakeVisible (intervalLabels[(size_t) i]);

        noteLabels[(size_t) i].setText ("VOICE " + n,
                                        juce::dontSendNotification);
        noteLabels[(size_t) i].setColour (
            juce::Label::textColourId, muted);
        noteLabels[(size_t) i].setJustificationType (
            juce::Justification::centred);
        addAndMakeVisible (noteLabels[(size_t) i]);

        setupSlider (interval[(size_t) i], "interval" + n, "Interval", false);
        setupSlider (level[(size_t) i], "level" + n, "Level", true);
        setupSlider (pan[(size_t) i], "pan" + n, "Pan", false);

        sliderAttachments[(size_t) ai++] =
            std::make_unique<SliderAttachment> (
                processor.parameters, "interval" + n, interval[(size_t) i]);

        sliderAttachments[(size_t) ai++] =
            std::make_unique<SliderAttachment> (
                processor.parameters, "level" + n, level[(size_t) i]);

        sliderAttachments[(size_t) ai++] =
            std::make_unique<SliderAttachment> (
                processor.parameters, "pan" + n, pan[(size_t) i]);

        interval[(size_t) i].onValueChange = [this, i]
        {
            intervalLabels[(size_t) i].setText (
                juce::String ((int) interval[(size_t) i].getValue()) + " st",
                juce::dontSendNotification);
        };

        intervalDown[(size_t) i].setButtonText ("-");
        intervalUp[(size_t) i].setButtonText ("+");
        styleButton (intervalDown[(size_t) i]);
        styleButton (intervalUp[(size_t) i]);

        intervalDown[(size_t) i].onClick = [this, i]
        {
            saveUndoState();
            interval[(size_t) i].setValue (
                interval[(size_t) i].getValue() - 1.0);
        };

        intervalUp[(size_t) i].onClick = [this, i]
        {
            saveUndoState();
            interval[(size_t) i].setValue (
                interval[(size_t) i].getValue() + 1.0);
        };

        addAndMakeVisible (intervalDown[(size_t) i]);
        addAndMakeVisible (intervalUp[(size_t) i]);
    }

    const juce::String modeNames[] { "SCALE", "CHORD", "FIXED" };

    for (int i = 0; i < 3; ++i)
    {
        modeButtons[(size_t) i].setButtonText (modeNames[i]);
        styleButton (modeButtons[(size_t) i]);

        modeButtons[(size_t) i].onClick = [this, i]
        {
            saveUndoState();
            if (auto* p = processor.parameters.getParameter ("mode"))
                p->setValueNotifyingHost ((float) i / 2.0f);
            updateModeButtons();
        };

        addAndMakeVisible (modeButtons[(size_t) i]);
    }

    manualButton.setButtonText ("MANUAL");
    autoButton.setButtonText ("AUTO");
    sourcePlus.setButtonText ("+");

    for (auto* b : { &manualButton, &autoButton, &sourcePlus })
    {
        styleButton (*b);
        addAndMakeVisible (*b);
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
        source.setSelectedId (source.getSelectedId() == 1 ? 2 : 1,
                              juce::sendNotificationSync);
        updateSourceButtons();
    };

    const juce::String latencyNames[] { "LOW", "MID", "HIGH" };

    for (int i = 0; i < 3; ++i)
    {
        latencyButtons[(size_t) i].setButtonText (latencyNames[i]);
        styleButton (latencyButtons[(size_t) i]);

        latencyButtons[(size_t) i].onClick = [this, i]
        {
            saveUndoState();
            if (auto* p = processor.parameters.getParameter ("latency"))
                p->setValueNotifyingHost ((float) i / 2.0f);
            updateLatencyButtons();
        };

        addAndMakeVisible (latencyButtons[(size_t) i]);
    }

    for (int i = 0; i < 4; ++i)
    {
        countButtons[(size_t) i].setButtonText (juce::String (i + 1));
        styleButton (countButtons[(size_t) i]);

        countButtons[(size_t) i].onClick = [this, i]
        {
            saveUndoState();
            if (auto* p = processor.parameters.getParameter ("voices"))
                p->setValueNotifyingHost ((float) i / 3.0f);
            updateVoiceButtons();
        };

        addAndMakeVisible (countButtons[(size_t) i]);
    }

    for (auto* b : { &undo, &redo, &copy, &paste, &reset,
                     &bypass, &resetWindow })
    {
        styleButton (*b);
        addAndMakeVisible (*b);
    }

    undo.onClick = [this] { performUndo(); };
    redo.onClick = [this] { performRedo(); };

    copy.onClick = [this]
    {
        std::unique_ptr<juce::XmlElement> xml (
            processor.parameters.copyState().createXml());
        if (xml != nullptr)
            juce::SystemClipboard::copyTextToClipboard (xml->toString());
    };

    paste.onClick = [this]
    {
        std::unique_ptr<juce::XmlElement> xml (
            juce::XmlDocument::parse (
                juce::SystemClipboard::getTextFromClipboard()));

        if (xml != nullptr
            && xml->hasTagName (processor.parameters.state.getType().toString()))
        {
            saveUndoState();
            processor.parameters.replaceState (
                juce::ValueTree::fromXml (*xml));
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
        auto* p = processor.parameters.getParameter ("bypass");

        if (p != nullptr)
            p->setValueNotifyingHost (p->getValue() >= 0.5f ? 0.0f : 1.0f);

        const bool active =
            processor.parameters.getRawParameterValue ("bypass")->load() >= 0.5f;

        bypass.setButtonText (active ? "BYPASSED" : "BYPASS");
        styleButton (bypass, active);
    };

    resetWindow.onClick = [this] { setSize (1100, 700); };

    auto undoOnDrag = [this] (juce::Slider& s)
    {
        s.onDragStart = [this] { saveUndoState(); };
    };

    for (auto* s : globalSliders)
        undoOnDrag (*s);

    for (int i = 0; i < 4; ++i)
    {
        undoOnDrag (interval[(size_t) i]);
        undoOnDrag (level[(size_t) i]);
        undoOnDrag (pan[(size_t) i]);
    }

    updateModeButtons();
    updateVoiceButtons();
    updateLatencyButtons();
    updateSourceButtons();
    updatePitchReadout();
}

void RrHarmonizerAudioProcessorEditor::setupSlider
    (juce::Slider& s, const juce::String& id,
     const juce::String& label, bool rotary)
{
    s.setName (label);
    s.setTooltip (label);
    s.setColour (juce::Slider::textBoxTextColourId, white);
    s.setColour (juce::Slider::textBoxBackgroundColourId, panel);
    s.setColour (juce::Slider::textBoxOutlineColourId, buttonBg);
    s.setColour (juce::Slider::rotarySliderFillColourId, accent);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, buttonBg);
    s.setColour (juce::Slider::thumbColourId, accent);
    s.setColour (juce::Slider::trackColourId, accent);
    s.setColour (juce::Slider::backgroundColourId, buttonBg);

    if (rotary)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 20);
    }
    else
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 42, 20);
    }

    addAndMakeVisible (s);
}

void RrHarmonizerAudioProcessorEditor::setupCombo
    (juce::ComboBox& c, const juce::String& id,
     const juce::StringArray& items)
{
    c.clear();

    for (int i = 0; i < items.size(); ++i)
        c.addItem (items[i], i + 1);

    c.setTooltip (id);
    c.setColour (juce::ComboBox::backgroundColourId, buttonBg);
    c.setColour (juce::ComboBox::textColourId, white);
    c.setColour (juce::ComboBox::outlineColourId, muted);
    c.setColour (juce::ComboBox::arrowColourId, accent);
    addAndMakeVisible (c);
}

void RrHarmonizerAudioProcessorEditor::styleButton
    (juce::TextButton& b, bool selected)
{
    b.setClickingTogglesState (false);
    b.setColour (juce::TextButton::buttonColourId,
                 selected ? accent : buttonBg);
    b.setColour (juce::TextButton::buttonOnColourId, accent);
    b.setColour (juce::TextButton::textColourOffId,
                 selected ? bg : white);
    b.setColour (juce::TextButton::textColourOnId, bg);
    b.setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void RrHarmonizerAudioProcessorEditor::updateModeButtons()
{
    const int mode = juce::jlimit (0, 2, (int) std::round (
        processor.parameters.getRawParameterValue ("mode")->load() * 2.0f));

    for (int i = 0; i < 3; ++i)
        styleButton (modeButtons[(size_t) i], i == mode);
}

void RrHarmonizerAudioProcessorEditor::updateVoiceButtons()
{
    const int voices = juce::jlimit (1, 4, 1 + (int) std::round (
        processor.parameters.getRawParameterValue ("voices")->load() * 3.0f));

    for (int i = 0; i < 4; ++i)
        styleButton (countButtons[(size_t) i], i + 1 == voices);
}

void RrHarmonizerAudioProcessorEditor::updateLatencyButtons()
{
    const int value = juce::jlimit (0, 2, (int) std::round (
        processor.parameters.getRawParameterValue ("latency")->load() * 2.0f));

    for (int i = 0; i < 3; ++i)
        styleButton (latencyButtons[(size_t) i], i == value);
}

void RrHarmonizerAudioProcessorEditor::updateSourceButtons()
{
    const bool manual = source.getSelectedId() == 2;
    styleButton (manualButton, manual);
    styleButton (autoButton, ! manual);
}

void RrHarmonizerAudioProcessorEditor::updatePitchReadout()
{
    const int midi = processor.getDetectedMidi();

    if (midi >= 0 && midi <= 127)
    {
        pitchReadout.setText ("PITCH: " + noteName (midi),
                              juce::dontSendNotification);
        statusReadout.setText ("PITCH DETECTED",
                               juce::dontSendNotification);
    }
    else
    {
        pitchReadout.setText ("PITCH: --", juce::dontSendNotification);
        statusReadout.setText ("WAITING FOR INPUT",
                               juce::dontSendNotification);
    }
}

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

    for (auto* p : processor.getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());

    updateModeButtons();
    updateVoiceButtons();
    updateLatencyButtons();
    updateSourceButtons();
    updatePitchReadout();

    bypass.setButtonText ("BYPASS");
    styleButton (bypass);
}

void RrHarmonizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    const int margin = 10;
    const int top = 112;
    const int gap = 8;
    const int panelHeight = getHeight() - top - margin;

    const int usableWidth = getWidth() - margin * 2 - gap * 4;
    const int widths[] {
        usableWidth * 23 / 100,
        usableWidth * 25 / 100,
        usableWidth * 27 / 100,
        usableWidth * 12 / 100,
        0
    };

    int remaining = usableWidth;
    for (int i = 0; i < 4; ++i)
        remaining -= widths[i];

    const int panelWidths[] {
        widths[0], widths[1], widths[2], widths[3], remaining
    };

    int x = margin;

    for (int w : panelWidths)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (
            (float) x, (float) top, (float) w,
            (float) panelHeight, 10.0f);
        x += w + gap;
    }

    g.setColour (panel);
    g.fillRoundedRectangle (
        10.0f, 8.0f, (float) getWidth() - 20.0f, 62.0f, 10.0f);

    const float output = juce::jlimit (0.0f, 1.0f, processor.getOutputLevel());

    const int meterX = getWidth() - 118;
    const int meterY = 49;

    g.setColour (buttonBg);
    g.fillRoundedRectangle (
        (float) meterX, (float) meterY, 90.0f, 7.0f, 3.5f);

    g.setColour (accent);
    g.fillRoundedRectangle (
        (float) meterX, (float) meterY, 90.0f * output, 7.0f, 3.5f);
}

void RrHarmonizerAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();
    const int margin = 10;
    const int gap = 8;
    const int top = 112;
    const int panelH = h - top - margin;
    const int usableW = w - margin * 2 - gap * 4;

    const int inputW = usableW * 23 / 100;
    const int harmonyW = usableW * 25 / 100;
    const int voicesW = usableW * 27 / 100;
    const int toneW = usableW * 12 / 100;
    const int outputW = usableW - inputW - harmonyW - voicesW - toneW;

    title.setBounds (22, 12, 280, 32);
    pitchTab.setBounds (24, 44, 300, 18);
    pitchReadout.setBounds (w - 290, 12, 170, 25);
    statusReadout.setBounds (w - 290, 40, 165, 18);

    const int utilityY = 76;
    const int utilityH = 28;
    const int utilityGap = 5;
    const int utilityW = (w - 2 * margin - 6 * utilityGap) / 7;

    juce::TextButton* utility[] {
        &bypass, &undo, &redo, &copy, &paste, &reset, &resetWindow
    };

    for (int i = 0; i < 7; ++i)
        utility[i]->setBounds (
            margin + i * (utilityW + utilityGap),
            utilityY, utilityW, utilityH);

    int x = margin;
    const int inner = 12;

    sectionInput.setBounds (x + inner, top + 10, inputW - inner * 2, 22);
    key.setBounds (x + inner, top + 42, inputW - inner * 2, 30);
    scale.setBounds (x + inner, top + 80, inputW - inner * 2, 30);
    chordSourceLabel.setBounds (x + inner, top + 122, inputW - inner * 2, 18);
    source.setBounds (x + inner, top + 144, inputW - inner * 2, 30);

    const int sourceGap = 5;
    const int sourceW = (inputW - inner * 2 - sourceGap) / 2;

    autoButton.setBounds (x + inner, top + 184, sourceW, 30);
    manualButton.setBounds (
        x + inner + sourceW + sourceGap, top + 184, sourceW, 30);
    sourcePlus.setBounds (x + inputW - 40, top + 10, 26, 24);
    noPitch.setBounds (x + inner, top + 232, inputW - inner * 2, 36);

    x += inputW + gap;

    sectionHarmony.setBounds (x + inner, top + 10, harmonyW - inner * 2, 22);

    const int modeGap = 5;
    const int modeW = (harmonyW - inner * 2 - modeGap * 2) / 3;

    for (int i = 0; i < 3; ++i)
        modeButtons[(size_t) i].setBounds (
            x + inner + i * (modeW + modeGap),
            top + 42, modeW, 30);

    const int controlGap = 5;
    const int controlW = (harmonyW - inner * 2 - controlGap * 2) / 3;
    const int controlY1 = top + 84;
    const int controlY2 = top + panelH / 2;
    const int controlH = juce::jmax (80, panelH / 3 - 10);

    gate.setBounds (x + inner, controlY1, controlW, controlH);
    response.setBounds (x + inner + controlW + controlGap,
                        controlY1, controlW, controlH);
    pitchLock.setBounds (x + inner + 2 * (controlW + controlGap),
                         controlY1, controlW, controlH);

    glide.setBounds (x + inner, controlY2, controlW, controlH);
    humanize.setBounds (x + inner + controlW + controlGap,
                        controlY2, controlW, controlH);
    formant.setBounds (x + inner + 2 * (controlW + controlGap),
                       controlY2, controlW, controlH);

    x += harmonyW + gap;

    sectionVoices.setBounds (x + inner, top + 10, voicesW - inner * 2, 22);

    const int countGap = 5;
    const int countW = (voicesW - inner * 2 - countGap * 3) / 4;

    for (int i = 0; i < 4; ++i)
        countButtons[(size_t) i].setBounds (
            x + inner + i * (countW + countGap), top + 42, countW, 30);

    noteHeading.setBounds (x + inner, top + 80, voicesW - inner * 2, 18);

    const int voiceGap = 5;
    const int oneW = (voicesW - inner * 2 - voiceGap * 3) / 4;
    const int vTop = top + 104;
    const int vH = panelH - 116;

    for (int i = 0; i < 4; ++i)
    {
        const int vx = x + inner + i * (oneW + voiceGap);

        voiceLabels[(size_t) i].setBounds (vx, vTop, oneW, 22);
        intervalLabels[(size_t) i].setBounds (vx, vTop + 25, oneW, 20);

        intervalDown[(size_t) i].setBounds (vx, vTop + 49, oneW / 2 - 2, 27);
        intervalUp[(size_t) i].setBounds (
            vx + oneW / 2 + 2, vTop + 49, oneW / 2 - 2, 27);

        interval[(size_t) i].setBounds (vx, vTop + 82, oneW, 34);
        level[(size_t) i].setBounds (vx, vTop + 120, oneW, vH / 3);
        pan[(size_t) i].setBounds (vx, vTop + 120 + vH / 3, oneW, 42);
        noteLabels[(size_t) i].setBounds (vx, top + panelH - 24, oneW, 18);
    }

    x += voicesW + gap;

    const int toneInner = 5;
    const int toneSliderH = (panelH - 54) / 3;

    consonants.setBounds (x + toneInner, top + 10,
                          toneW - toneInner * 2, toneSliderH);
    dry.setBounds (x + toneInner, top + 14 + toneSliderH,
                   toneW - toneInner * 2, toneSliderH);
    harmony.setBounds (x + toneInner, top + 18 + toneSliderH * 2,
                       toneW - toneInner * 2, toneSliderH);

    x += toneW + gap;

    sectionOutput.setBounds (x + 8, top + 10, outputW - 16, 22);
    outHeading.setBounds (x + 8, top + 42, outputW - 16, 20);

    const int latencyGap = 6;
    const int latencyW = outputW - 16;

    for (int i = 0; i < 3; ++i)
        latencyButtons[(size_t) i].setBounds (
            x + 8, top + 72 + i * (30 + latencyGap), latencyW, 30);
}
