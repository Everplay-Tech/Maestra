#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "../Systems/Logger.h"

// Simple global oversampling / anti-alias stage using juce::dsp::Oversampling.
// Pattern: process(buffer) will:
//   1. Upsample buffer
//   2. Apply low-pass anti-alias filter via Oversampling's internal filters
//   3. Downsample back into the original buffer
//
// This gives real anti-aliasing and spectral smoothing,
// and is cheap enough to run post-mix.

class Oversampler
{
public:
    explicit Oversampler (Logger& loggerIn)
        : logger (loggerIn)
    {
    }

    Oversampler (const Oversampler&) = delete;
    Oversampler& operator= (const Oversampler&) = delete;
    Oversampler (Oversampler&&) = delete;
    Oversampler& operator= (Oversampler&&) = delete;

    void prepare (const juce::dsp::ProcessSpec& specIn)
    {
        processSpec = specIn;

        // We pick a fixed 2x oversampling for now (stages = 1).
        constexpr int numChannels = 2;
        constexpr int numStages   = 1; // 2^1 = 2x

        oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
            numChannels,
            numStages,
            juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            true // useIntegerLatency
        );

        oversampling->initProcessing ((int) specIn.maximumBlockSize);
        oversampling->reset();

        prepared.store (true, std::memory_order_release);
    }

    void reset()
    {
        if (oversampling != nullptr)
            oversampling->reset();
    }

    // Global enable/disable
    void setEnabled (bool shouldEnable)
    {
        enabled.store (shouldEnable, std::memory_order_release);
    }

    bool isEnabled() const
    {
        return enabled.load (std::memory_order_acquire);
    }

    // In-place oversampling anti-alias:
    // This will upsample + filter + downsample the buffer.
    void process (juce::AudioBuffer<float>& buffer)
    {
        if (! prepared.load (std::memory_order_acquire))
            return;

        if (! enabled.load (std::memory_order_acquire))
            return;

        const auto numSamples = buffer.getNumSamples();
        if (numSamples <= 0)
            return;

        juce::dsp::AudioBlock<float> block (buffer);
        auto upBlock = oversampling->processSamplesUp (block);

        // If you ever insert non-linear processing, do it here on upBlock.
        juce::ignoreUnused (upBlock);

        oversampling->processSamplesDown (block);
        lastOversampleFactor.store (2, std::memory_order_relaxed);
    }

    struct OversamplerSnapshot
    {
        bool isPrepared = false;
        bool enabled = false;
        int factor = 1;
    };

    OversamplerSnapshot getSnapshot() const
    {
        OversamplerSnapshot s;
        s.isPrepared = prepared.load (std::memory_order_acquire);
        s.enabled    = enabled.load (std::memory_order_acquire);
        s.factor     = lastOversampleFactor.load (std::memory_order_relaxed);
        return s;
    }

private:
    juce::dsp::ProcessSpec processSpec {};
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    std::atomic<bool> prepared { false };
    std::atomic<bool> enabled { true };
    std::atomic<int> lastOversampleFactor { 1 };

    Logger& logger;
};
