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

bool OrchestraSynthApplication::validatePlatformSupport()
{
    const auto osName = juce::SystemStats::getOperatingSystemName();
    const auto osType = juce::SystemStats::getOperatingSystemType();
    const auto cpuVendor = juce::SystemStats::getCpuVendor();
    const auto cpuSpeed = juce::SystemStats::getCpuSpeedInMegahertz();
    const auto numCpus = juce::SystemStats::getNumCpus();
    const auto juceVersion = juce::SystemStats::getJUCEVersion();

    logger.log (Logger::LogLevel::Info,
                "JUCE version: " + juceVersion
                + ", OS: " + (osName.isNotEmpty() ? osName : juce::String ("Unknown"))
                + ", CPU vendor: " + (cpuVendor.isNotEmpty() ? cpuVendor : juce::String ("Unknown"))
                + ", cores: " + juce::String (numCpus)
                + ", clock (MHz): " + juce::String (cpuSpeed));

    bool osRecognised = osType != juce::SystemStats::OperatingSystemType::UnknownOS;
    bool cpuAvailable = numCpus > 0;

    if (! osRecognised)
        logger.log (Logger::LogLevel::Warning,
                    "JUCE could not recognise the current operating system; behaviour on newer/older hardware may be limited.");

    if (! cpuAvailable)
        logger.log (Logger::LogLevel::Warning,
                    "JUCE did not report a valid CPU configuration; real-time audio performance cannot be guaranteed.");

    if (cpuVendor.isEmpty())
        logger.log (Logger::LogLevel::Warning,
                    "CPU vendor information unavailable; consider updating firmware or platform diagnostics.");

    return osRecognised && cpuAvailable;
}

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

void OrchestraSynthApplication::systemRequestedQuit()
{
    quit();
}

void OrchestraSynthApplication::anotherInstanceStarted (const juce::String&)
{
}

void OrchestraSynthApplication::initialise (const juce::String&)
{
    logger.log (Logger::LogLevel::Info, "OrchestraSynth starting up");
    crashReporter.installGlobalHandler();
    perfMon.beginSession();

    if (! validatePlatformSupport())
    {
        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::WarningIcon,
            "Unsupported Platform",
            "OrchestraSynth could not verify JUCE compatibility on this hardware. "
            "Please update system drivers or contact support before continuing.");

        logger.log (Logger::LogLevel::Error,
                    "Application shutdown triggered due to unverified JUCE platform compatibility.");
        quit();
        return;
    }

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
