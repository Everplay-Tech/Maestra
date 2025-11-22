#include "SectionStripComponent.h"

SectionStripComponent::SectionStripComponent (OrchestraSynthEngine& engineIn,
                                              OrchestraSynthEngine::SectionIndex sectionIn,
                                              const juce::String& titleIn)
    : engine (engineIn),
      section (sectionIn),
      title (titleIn)
{
    titleLabel.setText (title, juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);

    voiceLabel.setJustificationType (juce::Justification::centredRight);
    voiceLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    voiceLabel.setText ("0 voices", juce::dontSendNotification);
    addAndMakeVisible (voiceLabel);

    articulationLabel.setText ("Articulation", juce::dontSendNotification);
    articulationLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    articulationLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (articulationLabel);

    articulationBadge.setColour (juce::Label::textColourId, juce::Colours::white);
    articulationBadge.setColour (juce::Label::backgroundColourId, juce::Colours::darkgrey.brighter (0.2f));
    articulationBadge.setJustificationType (juce::Justification::centred);
    articulationBadge.setText ("Sustain", juce::dontSendNotification);
    articulationBadge.setOpaque (true);
    addAndMakeVisible (articulationBadge);

    auto prepareRotarySlider = [this] (juce::Slider& slider,
                                       juce::Label& label,
                                       const juce::String& name,
                                       double min, double max, double step,
                                       double skew = 1.0)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
        slider.setRange (min, max, step);
        if (! juce::approximatelyEqual (skew, 1.0))
            slider.setSkewFactorFromMidPoint (skew);
        slider.addListener (this);
        slider.setName (name);
        slider.setWantsKeyboardFocus (false);
        addAndMakeVisible (slider);

        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible (label);
    };

    gainSlider.setSliderStyle (juce::Slider::LinearVertical);
    gainSlider.setRange (0.0, 1.5, 0.01);
    gainSlider.textFromValueFunction = [] (double value)
    {
        const double db = juce::Decibels::gainToDecibels ((float) value, -36.0f);
        return juce::String (db, 1) + " dB";
    };
    gainSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        auto trimmed = text.trimEnd();
        if (trimmed.endsWithIgnoreCase ("dB"))
            trimmed = trimmed.dropLastCharacters (2).trimEnd();
        const double db = trimmed.getDoubleValue();
        return juce::Decibels::decibelsToGain (db);
    };
    gainSlider.addListener (this);
    gainSlider.setName ("Gain");
    gainSlider.setWantsKeyboardFocus (false);
    addAndMakeVisible (gainSlider);

    gainLabel.setText ("Gain (dB)", juce::dontSendNotification);
    gainLabel.setJustificationType (juce::Justification::centred);
    gainLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (gainLabel);

    panSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    panSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 18);
    panSlider.setRange (-100.0, 100.0, 1.0);
    panSlider.setTextValueSuffix (" %");
    panSlider.addListener (this);
    panSlider.setName ("Pan");
    panSlider.setWantsKeyboardFocus (false);
    addAndMakeVisible (panSlider);

    panLabel.setText ("Pan %", juce::dontSendNotification);
    panLabel.setJustificationType (juce::Justification::centred);
    panLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (panLabel);

    prepareRotarySlider (cutoffSlider,   cutoffLabel,   "Cutoff Hz", 200.0, 20000.0, 1.0, 2000.0);
    prepareRotarySlider (resonanceSlider, resonanceLabel, "Resonance", 0.1, 1.5, 0.01);
    prepareRotarySlider (attackSlider,   attackLabel,   "Attack ms", 1.0, 2000.0, 1.0, 40.0);
    prepareRotarySlider (releaseSlider,  releaseLabel,  "Release ms", 10.0, 5000.0, 1.0, 200.0);
    prepareRotarySlider (reverbSlider,   reverbLabel,   "Reverb Send", 0.0, 1.0, 0.01);

    articulationBox.addItem ("Sustain", 1);
    articulationBox.addItem ("Staccato", 2);
    articulationBox.addItem ("Legato", 3);
    articulationBox.setWantsKeyboardFocus (false);
    articulationBox.onChange = [this]
    {
        auto p = engine.getSectionSnapshot (section).params;
        const int idx = articulationBox.getSelectedId() - 1;
        if (idx >= 0 && idx < OrchestraSynthEngine::numArticulations)
        {
            p.articulationIndex = idx;
            engine.setSectionParams (section, p);
        }
    };
    addAndMakeVisible (articulationBox);

    syncUIWithEngine();

    startTimerHz (10); // ~100ms updates for meter + param sync
}

SectionStripComponent::~SectionStripComponent()
{
    stopTimer();
    gainSlider.removeListener (this);
    panSlider.removeListener (this);
    cutoffSlider.removeListener (this);
    resonanceSlider.removeListener (this);
    attackSlider.removeListener (this);
    releaseSlider.removeListener (this);
    reverbSlider.removeListener (this);
}

void SectionStripComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    auto baseColour = juce::Colours::darkgrey.darker (0.2f);
    if (typingHighlight)
        baseColour = baseColour.brighter (0.2f);

    g.setColour (baseColour);
    g.fillRoundedRectangle (bounds, 6.0f);

    // Border
    g.setColour (juce::Colours::grey);
    g.drawRoundedRectangle (bounds.reduced (1.5f), 6.0f, 1.0f);

    auto content = getLocalBounds().reduced (6);

    // Meter region on the right
    auto meterWidth = 18;
    auto meterArea = content.removeFromRight (meterWidth);
    meterArea.reduce (2, 4);

    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (meterArea.toFloat(), 3.0f);

    const auto level = juce::jlimit (0.0f, 1.0f, meterLevel);
    auto fillHeight = (int) std::round (meterArea.getHeight() * level);
    auto filled = meterArea.withHeight (fillHeight).withY (meterArea.getBottom() - fillHeight);

    juce::Colour low = juce::Colours::green;
    juce::Colour high = juce::Colours::red;
    juce::Colour c = low.interpolatedWith (high, level);

    g.setColour (c);
    g.fillRoundedRectangle (filled.toFloat(), 2.0f);
}

void SectionStripComponent::resized()
{
    auto area = getLocalBounds().reduced (6);

    auto header = area.removeFromTop (28);
    titleLabel.setBounds (header.removeFromLeft (header.getWidth() * 2 / 3));
    voiceLabel.setBounds (header);

    auto articulationRow = area.removeFromTop (28);
    articulationLabel.setBounds (articulationRow.removeFromLeft (articulationRow.getWidth() / 2));
    articulationBox.setBounds (articulationRow.removeFromLeft (articulationRow.getWidth() - 80).reduced (2, 0));
    articulationBadge.setBounds (articulationRow.reduced (2));

    auto meterWidth = 20;
    area.removeFromRight (meterWidth);

    auto gainArea = area.removeFromLeft (70);
    auto gainLabelHeight = 18;
    auto gainLabelArea = gainArea.removeFromBottom (gainLabelHeight);
    gainSlider.setBounds (gainArea);
    gainLabel.setBounds (gainLabelArea);

    auto knobArea = area;
    auto knobHeight = knobArea.getHeight() / 2;

    auto layoutKnobRow = [] (juce::Rectangle<int> row,
                             juce::Slider& aSlider, juce::Label& aLabel,
                             juce::Slider& bSlider, juce::Label& bLabel,
                             juce::Slider& cSlider, juce::Label& cLabel)
    {
        auto slotWidth = row.getWidth() / 3;
        auto placeControl = [] (juce::Rectangle<int> slot,
                                juce::Slider& slider,
                                juce::Label& label)
        {
            const int labelHeight = 16;
            label.setBounds (slot.removeFromBottom (labelHeight));
            slider.setBounds (slot.reduced (2));
        };

        placeControl (row.removeFromLeft (slotWidth), aSlider, aLabel);
        placeControl (row.removeFromLeft (slotWidth), bSlider, bLabel);
        placeControl (row, cSlider, cLabel);
    };

    layoutKnobRow (knobArea.removeFromTop (knobHeight),
                   cutoffSlider, cutoffLabel,
                   resonanceSlider, resonanceLabel,
                   panSlider, panLabel);

    layoutKnobRow (knobArea,
                   attackSlider, attackLabel,
                   releaseSlider, releaseLabel,
                   reverbSlider, reverbLabel);
}

void SectionStripComponent::setTypingHighlight (bool shouldHighlight)
{
    if (typingHighlight == shouldHighlight)
        return;

    typingHighlight = shouldHighlight;
    repaint();
}

void SectionStripComponent::sliderValueChanged (juce::Slider* slider)
{
    auto p = engine.getSectionSnapshot (section).params;

    if (slider == &gainSlider)
        p.gain = (float) gainSlider.getValue();
    else if (slider == &panSlider)
        p.pan = (float) (panSlider.getValue() / 100.0);
    else if (slider == &cutoffSlider)
        p.cutoff = (float) cutoffSlider.getValue();
    else if (slider == &resonanceSlider)
        p.resonance = (float) resonanceSlider.getValue();
    else if (slider == &attackSlider)
        p.attackMs = (float) attackSlider.getValue();
    else if (slider == &releaseSlider)
        p.releaseMs = (float) releaseSlider.getValue();
    else if (slider == &reverbSlider)
        p.reverbSend = (float) reverbSlider.getValue();

    engine.setSectionParams (section, p);
}

void SectionStripComponent::timerCallback()
{
    auto snap = engine.getSectionSnapshot (section);
    updateMeterFromSnapshot (snap);

    // If engine parameters were changed from elsewhere (presets),
    // keep UI in sync without fighting the user too often.
    syncUIWithEngine();

    repaint();
}

void SectionStripComponent::syncUIWithEngine()
{
    auto s = engine.getSectionSnapshot (section);
    const auto& p = s.params;

    auto setIfDifferent = [] (juce::Slider& slider, double v)
    {
        if (std::abs (slider.getValue() - v) > 0.0001)
            slider.setValue (v, juce::dontSendNotification);
    };

    setIfDifferent (gainSlider,   p.gain);
    setIfDifferent (panSlider,    p.pan * 100.0);
    setIfDifferent (cutoffSlider, p.cutoff);
    setIfDifferent (resonanceSlider, p.resonance);
    setIfDifferent (attackSlider, p.attackMs);
    setIfDifferent (releaseSlider, p.releaseMs);
    setIfDifferent (reverbSlider, p.reverbSend);

    const int artId = p.articulationIndex + 1;
    if (articulationBox.getSelectedId() != artId)
        articulationBox.setSelectedId (artId, juce::dontSendNotification);

    articulationBadge.setText (articulationBox.getText(), juce::dontSendNotification);

    const auto voices = s.activeVoices;
    if (voices != lastActiveVoices || p.maxVoices != lastVoiceCapacity)
    {
        lastActiveVoices = voices;
        lastVoiceCapacity = p.maxVoices;
        voiceLabel.setText (juce::String (voices) + " / " + juce::String (p.maxVoices) + " voices",
                            juce::dontSendNotification);
    }
}

void SectionStripComponent::updateMeterFromSnapshot (const OrchestraSynthEngine::SectionStateSnapshot& s)
{
    const float ratio = s.params.maxVoices > 0
        ? (float) s.activeVoices / (float) s.params.maxVoices
        : 0.0f;

    meterLevel = juce::jlimit (0.0f, 1.0f, ratio);
}
