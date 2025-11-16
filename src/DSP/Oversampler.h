#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "../Systems/Logger.h"

class Oversampler
{
public:
    explicit Oversampler (Logger& loggerIn) : logger (loggerIn) {}

    Oversampler (const Oversampler&) = delete;
    Oversampler& operator= (const Oversampler&) = delete;
    Oversampler (Oversampler&&) = delete;
    Oversampler& operator= (Oversampler&&) = delete;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        currentSpec = spec;
        prepared.store (true, std::memory_order_release);
    }

    void reset() {}

    void beginOversampledBlock (juce::AudioBuffer<float>&)
    {
        lastOversampleFactor.store (2, std::memory_order_relaxed);
    }

    void endOversampledBlock (juce::AudioBuffer<float>&) {}

    struct OversamplerSnapshot
    {
        bool isPrepared = false;
        int factor = 1;
    };

    OversamplerSnapshot getSnapshot() const
    {
        OversamplerSnapshot s;
        s.isPrepared = prepared.load (std::memory_order_acquire);
        s.factor = lastOversampleFactor.load (std::memory_order_relaxed);
        return s;
    }

private:
    juce::dsp::ProcessSpec currentSpec {};
    std::atomic<bool> prepared { false };
    std::atomic<int> lastOversampleFactor { 1 };
    Logger& logger;
};
