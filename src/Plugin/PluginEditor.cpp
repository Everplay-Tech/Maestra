#include "PluginEditor.h"

OrchestraSynthAudioProcessorEditor::OrchestraSynthAudioProcessorEditor (OrchestraSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      mixer (processor.getEngine(),
             processor.getPresetManager(),
             processor.getPerformanceMonitor(),
             processor.getLogger())
{
}

void OrchestraSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void OrchestraSynthAudioProcessorEditor::resized()
{
    mixer.setBounds (getLocalBounds());
}
