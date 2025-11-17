#pragma once

#include <JuceHeader.h>
#include "../Engine/OrchestraSynthEngine.h"

class SectionStripComponent : public juce::Component,
                              private juce::Slider::Listener,
                              private juce::Timer
{
public:
    SectionStripComponent (OrchestraSynthEngine& engineIn,
                           OrchestraSynthEngine::SectionIndex sectionIn,
                           const juce::String& titleIn);

    ~SectionStripComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void sliderValueChanged (juce::Slider* slider) override;
    void timerCallback() override;

    void syncUIWithEngine();
    void applyToEngine();

    void updateMeterFromSnapshot (const OrchestraSynthEngine::SectionStateSnapshot& s);

    OrchestraSynthEngine& engine;
    OrchestraSynthEngine::SectionIndex section;
    juce::String title;

    juce::Label titleLabel;

    juce::Slider gainSlider;
    juce::Slider panSlider;
    juce::Slider cutoffSlider;
    juce::Slider reverbSlider;

    juce::ComboBox articulationBox;

    float meterLevel = 0.0f; // 0..1, based on activeVoices / maxVoices

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SectionStripComponent)
};
