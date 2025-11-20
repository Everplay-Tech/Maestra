#include "PluginEditor.h"

OrchestraSynthAudioProcessorEditor::OrchestraSynthAudioProcessorEditor (OrchestraSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p)
{
    mixer = processor.createMixerComponent();

    if (mixer != nullptr)
    {
        addAndMakeVisible (*mixer);
    }
    else
    {
        fallbackLabel.setText ("Mixer unavailable: JUCE UI initialisation failed on this platform.",
                               juce::dontSendNotification);
        fallbackLabel.setJustificationType (juce::Justification::centred);
        fallbackLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (fallbackLabel);
    }

    setSize (900, 600);
}

void OrchestraSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void OrchestraSynthAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    if (mixer != nullptr)
    {
        mixer->setBounds (bounds);
    }
    else
    {
        fallbackLabel.setBounds (bounds.reduced (32));
    }
}
