#pragma once

#include <JuceHeader.h>
#include "../Engine/OrchestraSynthEngine.h"
#include "../Systems/Logger.h"
#include "../Systems/CrashReporter.h"
#include "../Systems/PerformanceMonitor.h"
#include "../Systems/PresetManager.h"
#include "../Platform/AVAudioEngineManager.h"

class OrchestraSynthApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override          { return "OrchestraSynth"; }
    const juce::String getApplicationVersion() override       { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override                { return true; }

    void initialise (const juce::String& commandLine) override;
    void shutdown() override;

    void systemRequestedQuit() override           { quit(); }
    void anotherInstanceStarted (const juce::String&) override {}

private:
  class MixerComponent; // forward declaration

class OrchestraSynthApplication : public juce::JUCEApplication
{
public:
    // ... existing declarations ...

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name,
                    OrchestraSynthEngine& engine,
                    PresetManager& presetManager,
                    Logger& logger,
                    PerformanceMonitor& perfMon);

        void closeButtonPressed() override;

    private:
        std::unique_ptr<MixerComponent> mixerComponent;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;

    // shared systems (unchanged)
    Logger logger;
    CrashReporter crashReporter { logger };
    PerformanceMonitor perfMon { logger };
    PresetManager presetManager;
    OrchestraSynthEngine engine { presetManager, perfMon, logger };
    AVAudioEngineManager avAudioManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchestraSynthApplication)
};


    private:
        OrchestraSynthEngine& engine;
        PresetManager& presetManager;
        Logger& logger;
        PerformanceMonitor& perfMon;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;

    // Shared systems
    Logger logger;
    CrashReporter crashReporter { logger };
    PerformanceMonitor perfMon { logger };
    PresetManager presetManager;
    OrchestraSynthEngine engine { presetManager, perfMon, logger };
    AVAudioEngineManager avAudioManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchestraSynthApplication)
};
