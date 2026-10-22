#include "PluginEditor.h"

namespace
{
const juce::Colour bg (0xff38383d), panel (0xff2d2d32), panelDark (0xff19191d), header (0xff202025);
const juce::Colour lime (0xffb4d936), text (0xffd5d5d9), muted (0xff8e8e96), button (0xff505057), selected (0xff536b8d);
}

RrHarmonizerAudioProcessorEditor::RrHarmonizerAudioProcessorEditor (RrHarmonizerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (950, 410);
    auto label = [this] (juce::Label& l, const juce::String& s, float size = 11.0f, bool bold = false)
    {
        addAndMakeVisible (l); l.setText (s, juce::dontSendNotification);
        l.setColour (juce::Label::textColourId, text);
        l.setFont (juce::Font (juce::FontOptions (size).withStyle (bold ? juce::Font::bold : juce::Font::plain)));
    };
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
    label (noteHeading, "NOTE", 8.0f, true); label (levelHeading, "LEVEL", 8.0f, true);
    label (panHeading, "PAN", 8.0f, true); label (outHeading, "OUT", 8.0f, true);

    for (auto* b : { &undo, &redo, &copy, &paste, &reset, &bypass, &resetWindow })
    { addAndMakeVisible (*b); styleButton (*b); }
    paste.setEnabled (false);
    for (int i=0; i<3; ++i)
    {
        modeButtons[(size_t)i].setButtonText (juce::StringArray {"SCALE", "CHORD", "FIXED"}[i]);
        addAndMakeVisible (modeButtons[(size_t)i]); styleButton (modeButtons[(size_t)i], i == 0);
        modeButtons[(size_t)i].onClick = [this, i]
        {
            if (auto* param = processor.parameters.getParameter ("mode"))
                param->setValueNotifyingHost ((float)i / 2.0f);
            for (int j=0; j<3; ++j) styleButton (modeButtons[(size_t)j], j == i);
        };
    }
    for (auto* b : { &manualButton, &autoButton, &sourcePlus })
    { addAndMakeVisible (*b); styleButton (*b, b == &autoButton); }
    manualButton.onClick = [this] { styleButton (manualButton, true); styleButton (autoButton, false); };
    autoButton.onClick = [this] { styleButton (manualButton, false); styleButton (autoButton, true); };
    sourcePlus.onClick = [this] { source.setSelectedItemIndex (1 - source.getSelectedItemIndex()); };

    setupCombo (key, "key", {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"});
    setupCombo (scale, "scale", {"Major", "Natural Minor", "Harmonic Minor", "Dorian", "Mixolydian", "Chromatic"});
    setupCombo (source, "source", {"Music", "Manual"});
    comboAttachments[0] = std::make_unique<ComboAttachment> (processor.parameters, "key", key);
    comboAttachments[1] = std::make_unique<ComboAttachment> (processor.parameters, "scale", scale);
    comboAttachments[2] = std::make_unique<ComboAttachment> (processor.parameters, "source", source);

    const juce::StringArray latencyNames {"LOW", "MID", "HIGH"};
    for (int i=0; i<3; ++i)
    {
        latencyButtons[(size_t)i].setButtonText (latencyNames[i]); addAndMakeVisible (latencyButtons[(size_t)i]);
        styleButton (latencyButtons[(size_t)i], i == 1);
        latencyButtons[(size_t)i].onClick = [this, i]
        {
            if (auto* param = processor.parameters.getParameter ("latency")) param->setValueNotifyingHost ((float)i / 2.0f);
            for (int j=0; j<3; ++j) styleButton (latencyButtons[(size_t)j], j == i);
        };
    }
    for (int i=0; i<4; ++i)
    {
        countButtons[(size_t)i].setButtonText (juce::String (i+1)); addAndMakeVisible (countButtons[(size_t)i]);
        styleButton (countButtons[(size_t)i], i == 2);
        countButtons[(size_t)i].onClick = [this, i]
        {
            if (auto* param = processor.parameters.getParameter ("voices")) param->setValueNotifyingHost ((float)i / 3.0f);
            for (int j=0; j<4; ++j) styleButton (countButtons[(size_t)j], j == i);
        };
        intervalDown[(size_t)i].setButtonText ("-"); intervalUp[(size_t)i].setButtonText ("+");
        addAndMakeVisible (intervalDown[(size_t)i]); addAndMakeVisible (intervalUp[(size_t)i]);
        styleButton (intervalDown[(size_t)i]); styleButton (intervalUp[(size_t)i]);
        intervalDown[(size_t)i].onClick = [this, i] { interval[(size_t)i].setValue (interval[(size_t)i].getValue()-1.0); };
        intervalUp[(size_t)i].onClick = [this, i] { interval[(size_t)i].setValue (interval[(size_t)i].getValue()+1.0); };
        label (voiceLabels[(size_t)i], "V"+juce::String(i+1), 9.0f);
        label (intervalLabels[(size_t)i], juce::StringArray {"3rd above", "5th above", "4th below", "Octave above"}[i], 7.0f);
        label (noteLabels[(size_t)i], "--", 9.0f, true);
    }

    setupSlider (gate, "gate", "GATE"); setupSlider (response, "response", "CHORD RESP");
    setupSlider (pitchLock, "pitchLock", "PITCH LOCK"); setupSlider (glide, "glide", "GLIDE");
    setupSlider (humanize, "humanize", "HUMANIZE"); setupSlider (formant, "formant", "FORMANT");
    setupSlider (consonants, "consonants", "CONSONANTS"); setupSlider (dry, "dry", "DRY"); setupSlider (harmony, "harmony", "HARMONY");
    for (int i=0; i<4; ++i)
    {
        setupSlider (interval[(size_t)i], "interval"+juce::String(i+1), "INTERVAL", false);
        setupSlider (level[(size_t)i], "level"+juce::String(i+1), "LEVEL");
        setupSlider (pan[(size_t)i], "pan"+juce::String(i+1), "PAN");
    }
    int n = 0;
    for (auto pair : { std::pair<juce::Slider*,const char*>{&gate,"gate"}, {&response,"response"}, {&pitchLock,"pitchLock"},
         {&glide,"glide"}, {&humanize,"humanize"}, {&formant,"formant"}, {&consonants,"consonants"}, {&dry,"dry"}, {&harmony,"harmony"},
         {&interval[0],"interval1"}, {&level[0],"level1"}, {&pan[0],"pan1"}, {&interval[1],"interval2"}, {&level[1],"level2"}, {&pan[1],"pan2"},
         {&interval[2],"interval3"}, {&level[2],"level3"}, {&pan[2],"pan3"}, {&interval[3],"interval4"}, {&level[3],"level4"}, {&pan[3],"pan4"} })
        sliderAttachments[(size_t)n++] = std::make_unique<SliderAttachment> (processor.parameters, pair.second, *pair.first);

    bypass.setClickingTogglesState (true);
    bypass.onClick = [this] { if (auto* param = processor.parameters.getParameter ("bypass")) param->setValueNotifyingHost (bypass.getToggleState() ? 1.0f : 0.0f); };
    reset.onClick = [this] { resetAllParameters(); };
    // JUCE parameter state has no undo history configured in this prototype.
    undo.setEnabled (false); redo.setEnabled (false);
    copy.onClick = [this] { juce::SystemClipboard::copyText (processor.parameters.copyState().toXmlString()); };
    paste.onClick = [this] { auto xml = juce::parseXML (juce::SystemClipboard::getTextFromClipboard()); if (xml != nullptr) processor.parameters.replaceState (juce::ValueTree::fromXml (*xml)); };
    resetWindow.onClick = [this] { setSize (950, 410); centreAroundComponent (getParentComponent(), getWidth(), getHeight()); };
}

void RrHarmonizerAudioProcessorEditor::styleButton (juce::TextButton& b, bool isSelected)
{
    b.setColour (juce::TextButton::buttonColourId, isSelected ? selected : button);
    b.setColour (juce::TextButton::buttonOnColourId, selected);
    b.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    b.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    b.setConnectedEdges (juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
}
void RrHarmonizerAudioProcessorEditor::setupSlider (juce::Slider& s, const juce::String& id, const juce::String& name, bool rotary)
{
    addAndMakeVisible (s); s.setName (id);
    s.setSliderStyle (rotary ? juce::Slider::RotaryHorizontalVerticalDrag : juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 48, 13);
    s.setColour (juce::Slider::rotarySliderFillColourId, lime); s.setColour (juce::Slider::thumbColourId, lime);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, panelDark); s.setColour (juce::Slider::trackColourId, panelDark);
    s.setColour (juce::Slider::textBoxTextColourId, text); s.setColour (juce::Slider::textBoxBackgroundColourId, panel);
    s.setTooltip (name);
}
void RrHarmonizerAudioProcessorEditor::setupCombo (juce::ComboBox& c, const juce::String& id, const juce::StringArray& options)
{
    addAndMakeVisible (c); c.setName (id); for (int i=0; i<options.size(); ++i) c.addItem (options[i], i+1);
    c.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff555963));
    c.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    c.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff202025));
}
void RrHarmonizerAudioProcessorEditor::resetAllParameters()
{
    const juce::StringArray ids {"mode","key","scale","source","voices","latency","gate","response","pitchLock","glide","humanize","formant","consonants","dry","harmony","bypass",
        "interval1","level1","pan1","interval2","level2","pan2","interval3","level3","pan3","interval4","level4","pan4"};
    for (const auto& id : ids) if (auto* param = processor.parameters.getParameter (id)) param->setValueNotifyingHost (param->getDefaultValue());
    bypass.setToggleState (false, juce::dontSendNotification);
}
void RrHarmonizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (header); g.fillRect (0,0,getWidth(),38);
    g.setColour (juce::Colour (0xff27272c)); g.fillRect (0,38,getWidth(),38);
    g.setColour (lime); g.fillRect (10,48,3,18); g.fillRect (10,75,getWidth()-20,1);
    g.setColour (panel); g.fillRoundedRectangle (54,80,137,235,7); g.fillRoundedRectangle (195,80,303,235,7);
    g.fillRoundedRectangle (501,80,329,235,7); g.fillRoundedRectangle (834,80,58,235,7);
    g.fillRoundedRectangle (54,318,294,74,7); g.fillRoundedRectangle (352,318,116,74,7);
    g.setColour (panelDark); g.fillRoundedRectangle (62,98,122,56,5); g.fillRoundedRectangle (202,162,202,46,4);
    g.setColour (muted); g.setFont (juce::Font (juce::FontOptions (8.0f)));
    g.drawText ("no pitch",65,132,116,12,juce::Justification::centred);
    g.setColour (juce::Colour (0xff4a4a50)); g.fillRoundedRectangle (71,147,104,6,3);
    g.setColour (lime); g.fillRoundedRectangle (71,147,1,6,3);
    g.setColour (text); g.setFont (juce::Font (juce::FontOptions (8.0f)));
    g.drawText ("CHORD SOURCE",202,218,100,12,juce::Justification::left);
    g.drawText ("COUNT",510,103,40,10,juce::Justification::left);
    g.drawText ("NOTE",672,96,40,12,juce::Justification::centred);
    g.drawText ("LEVEL",719,96,48,12,juce::Justification::centred);
    g.drawText ("PAN",770,96,42,12,juce::Justification::centred);
    g.drawText ("OUT",843,83,36,12,juce::Justification::centred);
    // Output meter frame and ticks, matching the reference panel.
    g.setColour (panelDark); g.fillRoundedRectangle (855,103,25,208,3);
    g.setColour (muted); g.setFont (juce::Font (juce::FontOptions (8.0f)));
    for (int i=0; i<5; ++i) g.drawText (juce::StringArray {"0","-6","-12","-20","-40"}[i],836,109+i*43,18,10,juce::Justification::right);
    g.setColour (juce::Colour (0xff34343a)); g.drawRect (861,109,13,198,1);
    g.setColour (text); g.setFont (juce::Font (juce::FontOptions (8.0f)));
    g.drawText ("Rr Audio",18,392,80,10,juce::Justification::left);
}
void RrHarmonizerAudioProcessorEditor::resized()
{
    title.setBounds (32,5,150,25); pitchTab.setBounds (120,43,50,24);
    undo.setBounds (508,47,40,23); redo.setBounds (552,47,40,23); copy.setBounds (597,47,43,23); paste.setBounds (644,47,45,23);
    reset.setBounds (693,47,45,23); bypass.setBounds (743,47,72,23); resetWindow.setBounds (819,47,107,23);
    sectionInput.setBounds (64,83,70,12); sectionHarmony.setBounds (203,83,100,12); sectionVoices.setBounds (509,83,80,12);
    sectionTone.setBounds (64,321,60,12); sectionOutput.setBounds (362,321,70,12);
    pitchReadout.setBounds (212,164,180,38); statusReadout.setBounds (202,147,260,12); noPitch.setBounds (70,128,106,12);
    modeButtons[0].setBounds (202,99,92,20); modeButtons[1].setBounds (299,99,92,20); modeButtons[2].setBounds (396,99,92,20);
    manualButton.setBounds (202,124,58,20); autoButton.setBounds (263,124,39,20);
    key.setBounds (307,124,54,20); scale.setBounds (365,124,123,20);
    source.setBounds (202,233,168,20); sourcePlus.setBounds (374,233,30,20);
    for (int i=0;i<3;++i) latencyButtons[(size_t)i].setBounds (62+i*43,165,38,19);
    gate.setBounds (79,202,76,90); response.setBounds (421,156,58,77);
    for (int i=0;i<4;++i)
    {
        countButtons[(size_t)i].setBounds (572+i*31,99,26,19);
        const int y=132+i*43;
        voiceLabels[(size_t)i].setBounds (510,y+7,22,14); intervalDown[(size_t)i].setBounds (530,y,20,18); intervalUp[(size_t)i].setBounds (553,y,20,18);
        intervalLabels[(size_t)i].setBounds (530,y+19,68,12); interval[(size_t)i].setBounds (598,y+1,50,27);
        noteLabels[(size_t)i].setBounds (672,y+8,40,14); level[(size_t)i].setBounds (716,y-1,48,39); pan[(size_t)i].setBounds (766,y-1,48,39);
    }
    pitchLock.setBounds (62,332,48,57); glide.setBounds (120,332,48,57); humanize.setBounds (178,332,48,57);
    formant.setBounds (236,332,48,57); consonants.setBounds (294,332,48,57);
    dry.setBounds (359,332,48,57); harmony.setBounds (414,332,48,57);
}
