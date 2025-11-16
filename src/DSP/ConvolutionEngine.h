#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "../Systems/Logger.h"

class ConvolutionEngine
{
public:
    explicit ConvolutionEngine (Logger& loggerIn) : logger (loggerIn) {}

    ConvolutionEngine (const ConvolutionEngine&) = delete;
    ConvolutionEngine& operator= (const ConvolutionEngine&) = delete;
    ConvolutionEngine (ConvolutionEngine&&) = delete;
    ConvolutionEngine& operator= (ConvolutionEngine&&) = delete;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        juce::dsp::ProcessSpec s = spec;
        convolution.prepare (s);
        prepared.store (true, std::memory_order_release);
    }

    void reset()
    {
        convolution.reset();
    }

    void loadImpulseResponse (const juce::File& file)
    {
        auto irBuffer = loader.loadImpulse (file);
        // In a full implementation, turn this into a convolution IR
        juce::ignoreUnused (irBuffer);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        if (! prepared.load (std::memory_order_acquire))
            return;

        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        convolution.process (ctx);
    }

private:
    std::atomic<bool> prepared { false };
    juce::dsp::Convolution convolution;
    Logger& logger;

    class Loader
    {
    public:
        Loader() = default;

        Loader (const Loader&) = delete;
        Loader& operator= (const Loader&) = delete;
        Loader (Loader&&) = delete;
        Loader& operator= (Loader&&) = delete;

        juce::AudioBuffer<float> loadImpulse (const juce::File& file)
        {
            juce::AudioBuffer<float> buffer;
            if (! file.existsAsFile())
                return buffer;

            juce::AudioFormatManager fm;
            fm.registerBasicFormats();

            std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
            if (reader == nullptr)
                return buffer;

            buffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read (&buffer, 0, (int) reader->lengthInSamples, 0, true, true);
            return buffer;
        }
    };

    Loader loader;
};
