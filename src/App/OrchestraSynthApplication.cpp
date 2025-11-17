#include "OrchestraSynthApplication.h"
#include "../UI/MixerComponent.h"

void OrchestraSynthApplication::initialise (const juce::String&)
{
    logger.log (Logger::LogLevel::Info, "OrchestraSynth starting up");
    crashReporter.installGlobalHandler();
    perfMon.beginSession();

    avAudioManager.start();

    mainWindow = std::make_unique<MainWindow> (
        getApplicationName(), engine, presetManager, logger, perfMon);
}

void OrchestraSynthApplication::shutdown()
{
    mainWindow = nullptr;
    avAudioManager.stop();
    perfMon.endSession();
    crashReporter.uninstallGlobalHandler();
}

OrchestraSynthApplication::MainWindow::MainWindow (juce::String name,
                                                   OrchestraSynthEngine& engineIn,
                                                   PresetManager& presetManagerIn,
                                                   Logger& loggerIn,
                                                   PerformanceMonitor& perfMonIn)
    : DocumentWindow (name,
                      juce::Colours::black,
                      DocumentWindow::allButtons)
{
    setUsingNativeTitleBar (true);

    mixerComponent = std::make_unique<MixerComponent> (
        engineIn, presetManagerIn, perfMonIn, loggerIn);

    setContentOwned (mixerComponent.release(), true);

    centreWithSize (900, 600);
    setResizable (true, true);
    setVisible (true);
}

void OrchestraSynthApplication::MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
