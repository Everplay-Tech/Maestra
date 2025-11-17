#pragma once

#include <JuceHeader.h>
#include "../Engine/OrchestraSynthEngine.h"
#include "../Systems/PresetManager.h"
#include "../Systems/PerformanceMonitor.h"
#include "../Systems/Logger.h"

class PresetBar : public juce::Component
{
public:
    PresetBar (OrchestraSynthEngine& engineIn,
               PresetManager& presetManagerIn,
               PerformanceMonitor& perfMonIn,
               Logger& loggerIn);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void refreshPresetList();
    void saveCurrentPreset();
    void loadSelectedPreset();
    void updateStatusText();

    OrchestraSynthEngine& engine;
    PresetManager& presetManager;
    PerformanceMonitor& perfMon;
    Logger& logger;

    juce::ComboBox presetBox;
    juce::TextButton saveButton { "Save" };
    juce::TextButton loadButton { "Load" };
    juce::TextEditor nameEditor;
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};
