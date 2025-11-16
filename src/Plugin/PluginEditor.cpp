#include "PluginEditor.h"

OrchestraSynthAudioProcessorEditor::OrchestraSynthAudioProcessorEditor (OrchestraSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (900, 600);

    titleLabel.setText ("OrchestraSynth\nPlugin UI placeholder",
                        juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);
}

void OrchestraSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);
}

void OrchestraSynthAudioProcessorEditor::resized()
{
    titleLabel.setBounds (getLocalBounds().reduced (20));
}
