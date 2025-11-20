#pragma once

#include <JuceHeader.h>
#include "../Engine/OrchestraSynthEngine.h"
#include "../Systems/PresetManager.h"
#include "../Systems/Logger.h"
#include "../Systems/PerformanceMonitor.h"

class OrchestraSynthAudioProcessor  : public juce::AudioProcessor
{
public:
    OrchestraSynthAudioProcessor();
    ~OrchestraSynthAudioProcessor() override = default;

    // AudioProcessor overrides
    const juce::String getName() const override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool hasEditor() const override                                        { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    double getTailLengthSeconds() const override                           { return 0.0; }

    int getNumPrograms() override                                          { return 1; }
    int getCurrentProgram() override                                       { return 0; }
    void setCurrentProgram (int) override                                  {}
    const juce::String getProgramName (int) override                       { return {}; }
    void changeProgramName (int, const juce::String&) override            {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    OrchestraSynthEngine& getEngine() noexcept                             { return engine; }
    PresetManager& getPresetManager() noexcept                             { return presetManager; }
    PerformanceMonitor& getPerformanceMonitor() noexcept                   { return perfMon; }
    Logger& getLogger() noexcept                                           { return logger; }

private:
    Logger logger;
    PerformanceMonitor perfMon { logger };
    PresetManager presetManager;
    OrchestraSynthEngine engine { presetManager, perfMon, logger };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchestraSynthAudioProcessor)
};
