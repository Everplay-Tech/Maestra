#include "MixerComponent.h"

MixerComponent::MixerComponent (OrchestraSynthEngine& engineIn,
                                PresetManager& presetManagerIn,
                                PerformanceMonitor& perfMonIn,
                                Logger& loggerIn)
    : engine (engineIn),
      presetManager (presetManagerIn),
      perfMon (perfMonIn),
      logger (loggerIn),
      presetBar (engineIn, presetManagerIn, perfMonIn, loggerIn),
      stringsStrip (engineIn, OrchestraSynthEngine::Strings,    "Strings"),
      brassStrip (engineIn,   OrchestraSynthEngine::Brass,      "Brass"),
      woodwindsStrip (engineIn, OrchestraSynthEngine::Woodwinds,"Woodwinds"),
      percussionStrip (engineIn, OrchestraSynthEngine::Percussion, "Percussion"),
      choirStrip (engineIn,   OrchestraSynthEngine::Choir,      "Choir")
{
    addAndMakeVisible (presetBar);
    addAndMakeVisible (stringsStrip);
    addAndMakeVisible (brassStrip);
    addAndMakeVisible (woodwindsStrip);
    addAndMakeVisible (percussionStrip);
    addAndMakeVisible (choirStrip);
}

void MixerComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void MixerComponent::resized()
{
    auto area = getLocalBounds();

    auto top = area.removeFromTop (40);
    presetBar.setBounds (top);

    auto stripArea = area.reduced (8, 4);
    int stripWidth = stripArea.getWidth() / 5;

    stringsStrip.setBounds    (stripArea.removeFromLeft (stripWidth).reduced (4));
    brassStrip.setBounds      (stripArea.removeFromLeft (stripWidth).reduced (4));
    woodwindsStrip.setBounds  (stripArea.removeFromLeft (stripWidth).reduced (4));
    percussionStrip.setBounds (stripArea.removeFromLeft (stripWidth).reduced (4));
    choirStrip.setBounds      (stripArea.reduced (4));
}
