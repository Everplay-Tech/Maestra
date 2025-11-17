#include "PresetBar.h"

PresetBar::PresetBar (OrchestraSynthEngine& engineIn,
                      PresetManager& presetManagerIn,
                      PerformanceMonitor& perfMonIn,
                      Logger& loggerIn)
    : engine (engineIn),
      presetManager (presetManagerIn),
      perfMon (perfMonIn),
      logger (loggerIn)
{
    presetBox.onChange = [this] { /* no-op, load is explicit */ };
    addAndMakeVisible (presetBox);

    nameEditor.setText ("Default", juce::dontSendNotification);
    addAndMakeVisible (nameEditor);

    saveButton.onClick = [this] { saveCurrentPreset(); };
    loadButton.onClick = [this] { loadSelectedPreset(); };
    addAndMakeVisible (saveButton);
    addAndMakeVisible (loadButton);

    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);

    refreshPresetList();
    updateStatusText();
}

void PresetBar::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);
}

void PresetBar::resized()
{
    auto area = getLocalBounds().reduced (4);

    auto left = area.removeFromLeft (area.getWidth() * 2 / 3);
    auto right = area;

    auto lineHeight = left.getHeight();

    presetBox.setBounds (left.removeFromLeft (left.getWidth() / 3));
    nameEditor.setBounds (left.removeFromLeft (left.getWidth() / 2).reduced (4, 0));
    saveButton.setBounds (left.removeFromLeft (60).reduced (2, 0));
    loadButton.setBounds (left.removeFromLeft (60).reduced (2, 0));

    statusLabel.setBounds (right.reduced (4, 0));
}

void PresetBar::refreshPresetList()
{
    presetBox.clear (juce::dontSendNotification);

    auto names = presetManager.getPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetBox.addItem (names[i], i + 1);

    if (presetBox.getNumItems() > 0 && presetBox.getSelectedId() == 0)
        presetBox.setSelectedId (1, juce::dontSendNotification);
}

void PresetBar::saveCurrentPreset()
{
    auto name = nameEditor.getText().trim();
    if (name.isEmpty())
        name = "Preset";

    engine.savePreset (name);
    logger.log (Logger::LogLevel::Info, "Saved preset: " + name);

    refreshPresetList();
    updateStatusText();
}

void PresetBar::loadSelectedPreset()
{
    auto id = presetBox.getSelectedId();
    auto name = presetBox.getItemText (presetBox.getSelectedItemIndex());

    if (id == 0 || name.isEmpty())
        return;

    engine.loadPreset (name);
    logger.log (Logger::LogLevel::Info, "Loaded preset: " + name);

    // ensure UI-controlled params stay in sync; SectionStrip components
    // pull updated params from engine in their timers.
    updateStatusText();
}

void PresetBar::updateStatusText()
{
    auto stats = perfMon.getSnapshot();
    juce::String text;
    text << "Block: "
         << juce::String (stats.lastBlockMs, 2) << " ms  (avg "
         << juce::String (stats.averageBlockMs, 2) << " ms), "
         << "Log entries: " << logger.getTotalCount();

    statusLabel.setText (text, juce::dontSendNotification);
}
