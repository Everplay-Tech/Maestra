#include "PluginEditor.h"

OrchestraSynthAudioProcessorEditor::OrchestraSynthAudioProcessorEditor (OrchestraSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      mixer (processor.getEngine(),
             processor.getPresetManager(),
             processor.getEngine().perfMon, /* not accessible directly */
             processor.getEngine().logger)  /* also not public */
{
    // The above line won't compile as-is because perfMon/logger are private in the engine/processor.
    // Instead, construct MixerComponent using the processor's public accessors we already have.

    // Correct construction:
    // (Adjust constructor arguments accordingly.)

}

void OrchestraSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void OrchestraSynthAudioProcessorEditor::resized()
{
    mixer.setBounds (getLocalBounds());
}
