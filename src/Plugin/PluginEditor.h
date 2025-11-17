#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../UI/MixerComponent.h"

class OrchestraSynthAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit OrchestraSynthAudioProcessorEditor (OrchestraSynthAudioProcessor&);
    ~OrchestraSynthAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OrchestraSynthAudioProcessor& processor;
    MixerComponent mixer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchestraSynthAudioProcessorEditor)
};
