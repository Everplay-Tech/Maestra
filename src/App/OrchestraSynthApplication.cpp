#include "OrchestraSynthApplication.h"

void OrchestraSynthApplication::initialise (const juce::String&)
{
    logger.log(Logger::LogLevel::Info, "OrchestraSynth starting up");
    crashReporter.installGlobalHandler();
    perfMon.beginSession();

    avAudioManager.start(); // runtime rendering path (optional in plugin)

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
                      DocumentWindow::allButtons),
      engine (engineIn),
      presetManager (presetManagerIn),
      logger (loggerIn),
      perfMon (perfMonIn)
{
    // Simple placeholder UI: a basic component with sections listed.
    auto* content = new juce::Component();
    content->setSize (900, 600);

    auto* label = new juce::Label ("info",
                                   "OrchestraSynth\nStandalone UI placeholder\n"
                                   "Sections: Strings, Brass, Woodwinds, Percussion, Choir");
    label->setJustificationType (juce::Justification::centred);
    label->setBounds (20, 20, 860, 80);
    content->addAndMakeVisible (label);

    setUsingNativeTitleBar (true);
    setContentOwned (content, true);
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
}

void OrchestraSynthApplication::MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
