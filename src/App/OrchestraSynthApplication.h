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
    const juce::String getApplicationName() override;
    const juce::String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed() override;

    ~OrchestraSynthApplication() override;

    void initialise (const juce::String& commandLine) override;
    void shutdown() override;

    void systemRequestedQuit() override           { quit(); }
    void anotherInstanceStarted (const juce::String&) override {}

private:
    class MainWindow;
    std::unique_ptr<MainWindow> mainWindow;

    // shared systems
    Logger logger;
    CrashReporter crashReporter { logger };
    PerformanceMonitor perfMon { logger };
    PresetManager presetManager;
    OrchestraSynthEngine engine { presetManager, perfMon, logger };
    AVAudioEngineManager avAudioManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchestraSynthApplication)
};
