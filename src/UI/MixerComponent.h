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
                    PresetManager& presetManagerIn);

    // ...

private:
    OrchestraSynthEngine& engine;
    PresetManager& presetManager;

    PresetBar presetBar;

    // ...
};
