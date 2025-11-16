#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class OrchestraSynthAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit OrchestraSynthAudioProcessorEditor (OrchestraSynthAudioProcessor&);
    ~OrchestraSynthAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OrchestraSynthAudioProcessor& processor;
    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchestraSynthAudioProcessorEditor)
};
