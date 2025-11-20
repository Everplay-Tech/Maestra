#pragma once

#include <JuceHeader.h>
#include "../Engine/OrchestraSynthEngine.h"
#include "../Systems/PresetManager.h"
#include "../Systems/PerformanceMonitor.h"
#include "../Systems/Logger.h"
#include "SectionStripComponent.h"
#include "PresetBar.h"

class MixerComponent : public juce::Component
{
public:
    MixerComponent (OrchestraSynthEngine& engineIn,
                    PresetManager& presetManagerIn,
                    PerformanceMonitor& perfMonIn,
                    Logger& loggerIn);

    void paint (juce::Graphics& g) override;
    void resized() override;

    // ...

private:
    OrchestraSynthEngine& engine;
    PresetManager& presetManager;
    PerformanceMonitor& perfMon;
    Logger& logger;

    PresetBar presetBar;
    SectionStripComponent stringsStrip;
    SectionStripComponent brassStrip;
    SectionStripComponent woodwindsStrip;
    SectionStripComponent percussionStrip;
    SectionStripComponent choirStrip;

    // ...

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerComponent)
};
