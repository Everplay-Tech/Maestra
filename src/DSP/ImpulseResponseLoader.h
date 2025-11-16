#pragma once

#include <JuceHeader.h>
#include <atomic>

class ImpulseResponseLoader
{
public:
    ImpulseResponseLoader() = default;

    ImpulseResponseLoader (const ImpulseResponseLoader&) = delete;
    ImpulseResponseLoader& operator= (const ImpulseResponseLoader&) = delete;
    ImpulseResponseLoader (ImpulseResponseLoader&&) = delete;
    ImpulseResponseLoader& operator= (ImpulseResponseLoader&&) = delete;

    struct LoadSnapshot
    {
        bool lastLoadSucceeded = false;
    };

    juce::AudioBuffer<float> load (const juce::File& file)
    {
        juce::AudioBuffer<float> buffer;
        success.store (false, std::memory_order_relaxed);

        if (! file.existsAsFile())
            return buffer;

        juce::AudioFormatManager fmt;
        fmt.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (fmt.createReaderFor (file));
        if (reader == nullptr)
            return buffer;

        buffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&buffer, 0, (int) reader->lengthInSamples, 0, true, true);
        success.store (true, std::memory_order_release);
        return buffer;
    }

    LoadSnapshot getSnapshot() const
    {
        LoadSnapshot s;
        s.lastLoadSucceeded = success.load (std::memory_order_acquire);
        return s;
    }

private:
    std::atomic<bool> success { false };
};
