#include "SectionStripComponent.h"

SectionStripComponent::SectionStripComponent (OrchestraSynthEngine& engineIn,
                                              OrchestraSynthEngine::SectionIndex sectionIn,
                                              const juce::String& titleIn)
    : engine (engineIn),
      section (sectionIn),
      title (titleIn)
{
    titleLabel.setText (title, juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    gainSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 48, 20);
    gainSlider.setRange (0.0, 1.5, 0.01);
    gainSlider.setName ("Gain");
    gainSlider.addListener (this);
    addAndMakeVisible (gainSlider);

    panSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    panSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 48, 20);
    panSlider.setRange (-1.0, 1.0, 0.01);
    panSlider.setName ("Pan");
    panSlider.addListener (this);
    addAndMakeVisible (panSlider);

    cutoffSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    cutoffSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    cutoffSlider.setRange (500.0, 20000.0, 1.0);
    cutoffSlider.setSkewFactorFromMidPoint (2000.0);
    cutoffSlider.setName ("Cutoff");
    cutoffSlider.addListener (this);
    addAndMakeVisible (cutoffSlider);

    reverbSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    reverbSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 48, 20);
    reverbSlider.setRange (0.0, 1.0, 0.01);
    reverbSlider.setName ("Rev");
    reverbSlider.addListener (this);
    addAndMakeVisible (reverbSlider);

    articulationBox.addItem ("Sustain", 1);
    articulationBox.addItem ("Staccato", 2);
    articulationBox.addItem ("Legato", 3);
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
    reverbSlider.removeListener (this);
}

void SectionStripComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey.darker (0.3f));

    // Border
    g.setColour (juce::Colours::grey);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 4.0f, 1.0f);

    // Meter region: right side vertical bar
    auto bounds = getLocalBounds().reduced (6);
    auto meterWidth = 16;
    auto meterArea = bounds.removeFromRight (meterWidth);

    g.setColour (juce::Colours::black);
    g.fillRect (meterArea);

    const auto level = juce::jlimit (0.0f, 1.0f, meterLevel);
    auto fillHeight = (int) (meterArea.getHeight() * level);
    auto filled = meterArea.withHeight (fillHeight).withY (meterArea.getBottom() - fillHeight);

    juce::Colour low = juce::Colours::green;
    juce::Colour high = juce::Colours::red;
    juce::Colour c = low.interpolatedWith (high, level);

    g.setColour (c);
    g.fillRect (filled);
}

void SectionStripComponent::resized()
{
    auto area = getLocalBounds().reduced (6);

    auto topHeight = 24;
    titleLabel.setBounds (area.removeFromTop (topHeight));

    auto artHeight = 24;
    articulationBox.setBounds (area.removeFromTop (artHeight).reduced (2, 0));

    // Leave meter strip on the right
    area.removeFromRight (20);

    auto rowHeight = area.getHeight() / 4;
    gainSlider.setBounds (area.removeFromTop (rowHeight).reduced (4));
    panSlider.setBounds (area.removeFromTop (rowHeight).reduced (4));
    cutoffSlider.setBounds (area.removeFromTop (rowHeight).reduced (4));
    reverbSlider.setBounds (area.removeFromTop (rowHeight).reduced (4));
}

void SectionStripComponent::sliderValueChanged (juce::Slider* slider)
{
    auto p = engine.getSectionSnapshot (section).params;

    if (slider == &gainSlider)
        p.gain = (float) gainSlider.getValue();
    else if (slider == &panSlider)
        p.pan = (float) panSlider.getValue();
    else if (slider == &cutoffSlider)
        p.cutoff = (float) cutoffSlider.getValue();
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
    setIfDifferent (panSlider,    p.pan);
    setIfDifferent (cutoffSlider, p.cutoff);
    setIfDifferent (reverbSlider, p.reverbSend);

    const int artId = p.articulationIndex + 1;
    if (articulationBox.getSelectedId() != artId)
        articulationBox.setSelectedId (artId, juce::dontSendNotification);
}

void SectionStripComponent::updateMeterFromSnapshot (const OrchestraSynthEngine::SectionStateSnapshot& s)
{
    const float ratio = s.params.maxVoices > 0
        ? (float) s.activeVoices / (float) s.params.maxVoices
        : 0.0f;

    meterLevel = juce::jlimit (0.0f, 1.0f, ratio);
}
