#include "OrchestraSynthApplication.h"
#include "../UI/MixerComponent.h"

class OrchestraSynthApplication::MainWindow : public juce::DocumentWindow
{
public:
    MainWindow (juce::String name,
                OrchestraSynthEngine& engine,
                PresetManager& presetManager,
                Logger& logger,
                PerformanceMonitor& perfMon)
        : DocumentWindow (name, juce::Colours::black, DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);

        mixerComponent = std::make_unique<MixerComponent> (
            engine, presetManager, perfMon, logger);

        setContentOwned (mixerComponent.release(), true);

        centreWithSize (900, 600);
        setResizable (true, true);
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    std::unique_ptr<MixerComponent> mixerComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

OrchestraSynthApplication::~OrchestraSynthApplication() = default;

const juce::String OrchestraSynthApplication::getApplicationName()
{
    return "OrchestraSynth";
}

const juce::String OrchestraSynthApplication::getApplicationVersion()
{
    return "0.1.0";
}

bool OrchestraSynthApplication::moreThanOneInstanceAllowed()
{
    return true;
}

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
