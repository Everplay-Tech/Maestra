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

    void setTypingHighlight (bool shouldHighlight);

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
    juce::Label voiceLabel;
    juce::Label articulationLabel;

    juce::Slider gainSlider;
    juce::Slider panSlider;
    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider reverbSlider;

    juce::Label gainLabel;
    juce::Label panLabel;
    juce::Label cutoffLabel;
    juce::Label resonanceLabel;
    juce::Label attackLabel;
    juce::Label releaseLabel;
    juce::Label reverbLabel;

    juce::ComboBox articulationBox;
    juce::Label articulationBadge;

    float meterLevel = 0.0f; // 0..1, based on activeVoices / maxVoices
    bool typingHighlight = false;
    int lastActiveVoices = 0;
    int lastVoiceCapacity = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SectionStripComponent)
};
