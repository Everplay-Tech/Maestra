#include "PluginEditor.h"

OrchestraSynthAudioProcessorEditor::OrchestraSynthAudioProcessorEditor (OrchestraSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p)
{
    mixer = processor.createMixerComponent();
    jassert (mixer != nullptr);
    addAndMakeVisible (*mixer);
    setSize (900, 600);
}

void OrchestraSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void OrchestraSynthAudioProcessorEditor::resized()
{
    if (mixer != nullptr)
        mixer->setBounds (getLocalBounds());
}
