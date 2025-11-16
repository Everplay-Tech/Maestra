#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <array>
#include "../DSP/Oversampler.h"
#include "../DSP/ConvolutionEngine.h"
#include "../DSP/ImpulseResponseLoader.h"
#include "../Systems/PresetManager.h"
#include "../Systems/PerformanceMonitor.h"
#include "../Systems/Logger.h"

class OrchestraSynthEngine
{
public:
    static constexpr int numSections = 5;
    enum SectionIndex { Strings = 0, Brass, Woodwinds, Percussion, Choir };

    struct SectionParams
    {
        float gain = 0.8f;
        float pan = 0.0f;
        float cutoff = 12000.0f;
        float resonance = 0.7f;
        float attackMs = 5.0f;
        float releaseMs = 200.0f;
        float reverbSend = 0.3f;
        float oversampleFactor = 2.0f;
        int   maxVoices = 32;
    };

    struct SectionStateSnapshot
    {
        SectionParams params;
        int activeVoices = 0;
    };

    OrchestraSynthEngine (PresetManager& presetManager,
                          PerformanceMonitor& perfMon,
                          Logger& logger)
        : presetManager (presetManager),
          perfMon (perfMon),
          logger (logger),
          convolutionReverb (logger),
          oversampler (logger)
    {
        sectionParams[Strings].maxVoices    = 48;
        sectionParams[Brass].maxVoices      = 32;
        sectionParams[Woodwinds].maxVoices  = 32;
        sectionParams[Percussion].maxVoices = 32;
        sectionParams[Choir].maxVoices      = 32;
    }

    OrchestraSynthEngine (const OrchestraSynthEngine&) = delete;
    OrchestraSynthEngine& operator= (const OrchestraSynthEngine&) = delete;
    OrchestraSynthEngine (OrchestraSynthEngine&&) = delete;
    OrchestraSynthEngine& operator= (OrchestraSynthEngine&&) = delete;

    void prepare (double sampleRate, int samplesPerBlock)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
        spec.numChannels = 2;

        convolutionReverb.prepare (spec);
        oversampler.prepare (spec);
        internalSampleRate = sampleRate;
    }

    void reset()
    {
        convolutionReverb.reset();
        oversampler.reset();
    }

    void handleMidi (const juce::MidiBuffer& midi)
    {
        // Deterministic MIDI handling stub; extend with per-section voice alloc.
        lastMidiCount.store (midi.getNumEvents(), std::memory_order_relaxed);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        const auto numSamples = buffer.getNumSamples();
        perfMon.beginBlock();

        handleMidi (midi);
        buffer.clear();

        // Simple placeholder synthesis: one sine per section mixed into buffer
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);

        oversampler.beginOversampledBlock (buffer);

        for (int sec = 0; sec < numSections; ++sec)
        {
            const auto& p = sectionParams[(SectionIndex)sec];

            tempBuffer.setSize (2, numSamples, false, false, true);
            tempBuffer.clear();

            juce::dsp::AudioBlock<float> tempBlock (tempBuffer);
            auto sampleRate = internalSampleRate.load (std::memory_order_relaxed);
            if (sampleRate <= 0.0)
                sampleRate = 44100.0;

            auto hz = 220.0 * std::pow (2.0, sec); // just for differentiation

            for (int ch = 0; ch < tempBuffer.getNumChannels(); ++ch)
            {
                auto* data = tempBuffer.getWritePointer (ch);
                for (int n = 0; n < numSamples; ++n)
                {
                    const auto t = (phase[sec] + n) / sampleRate;
                    data[n] = (float) (std::sin (juce::MathConstants<double>::twoPi * hz * t) * p.gain);
                }
            }

            phase[sec] += numSamples;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addFrom (ch, 0, tempBuffer, ch, 0, numSamples);
        }

        convolutionReverb.process (buffer);
        oversampler.endOversampledBlock (buffer);

        perfMon.endBlock (buffer.getNumSamples());
    }

    void setSectionParams (SectionIndex index, const SectionParams& params)
    {
        sectionParams[index] = params;
    }

    SectionStateSnapshot getSectionSnapshot (SectionIndex index) const
    {
        SectionStateSnapshot s;
        s.params = sectionParams[index];
        s.activeVoices = 0; // extend when you add real voices
        return s;
    }

    void savePreset (const juce::String& name)
    {
        presetManager.savePreset (name, *this);
    }

    void loadPreset (const juce::String& name)
    {
        presetManager.loadPreset (name, *this);
    }

    // called by PresetManager
    void serialiseToValueTree (juce::ValueTree& dest) const
    {
        for (int sec = 0; sec < numSections; ++sec)
        {
            juce::String sectionName;
            switch (sec)
            {
                case Strings:    sectionName = "strings";    break;
                case Brass:      sectionName = "brass";      break;
                case Woodwinds:  sectionName = "woodwinds";  break;
                case Percussion: sectionName = "percussion"; break;
                case Choir:      sectionName = "choir";      break;
                default:         sectionName = "unknown";    break;
            }

            auto sectionTree = juce::ValueTree (juce::Identifier (sectionName));
            const auto& p = sectionParams[(SectionIndex)sec];

            auto setF = [&sectionTree](const char* key, float v)
            {
                sectionTree.setProperty (juce::Identifier (key), v, nullptr);
            };

            sectionTree.setProperty (juce::Identifier ("maxVoices"), p.maxVoices, nullptr);
            setF ("gain", p.gain);
            setF ("pan", p.pan);
            setF ("cutoff", p.cutoff);
            setF ("resonance", p.resonance);
            setF ("attackMs", p.attackMs);
            setF ("releaseMs", p.releaseMs);
            setF ("reverbSend", p.reverbSend);
            setF ("oversampleFactor", p.oversampleFactor);

            dest.addChild (sectionTree, -1, nullptr);
        }
    }

    void deserialiseFromValueTree (const juce::ValueTree& src)
    {
        auto loadSection = [this, &src](SectionIndex idx, const char* id)
        {
            auto t = src.getChildWithName (juce::Identifier (id));
            if (! t.isValid())
                return;

            auto& p = sectionParams[idx];
            p.maxVoices       = (int) t.getProperty (juce::Identifier ("maxVoices"), p.maxVoices);
            p.gain            = (float) t.getProperty (juce::Identifier ("gain"), p.gain);
            p.pan             = (float) t.getProperty (juce::Identifier ("pan"), p.pan);
            p.cutoff          = (float) t.getProperty (juce::Identifier ("cutoff"), p.cutoff);
            p.resonance       = (float) t.getProperty (juce::Identifier ("resonance"), p.resonance);
            p.attackMs        = (float) t.getProperty (juce::Identifier ("attackMs"), p.attackMs);
            p.releaseMs       = (float) t.getProperty (juce::Identifier ("releaseMs"), p.releaseMs);
            p.reverbSend      = (float) t.getProperty (juce::Identifier ("reverbSend"), p.reverbSend);
            p.oversampleFactor= (float) t.getProperty (juce::Identifier ("oversampleFactor"), p.oversampleFactor);
        };

        loadSection (Strings,    "strings");
        loadSection (Brass,      "brass");
        loadSection (Woodwinds,  "woodwinds");
        loadSection (Percussion, "percussion");
        loadSection (Choir,      "choir");
    }

private:
    PresetManager& presetManager;
    PerformanceMonitor& perfMon;
    Logger& logger;

    ConvolutionEngine convolutionReverb;
    Oversampler oversampler;
    ImpulseResponseLoader irLoader;

    std::array<SectionParams, numSections> sectionParams {};
    std::array<int64_t, numSections> phase { 0, 0, 0, 0, 0 };

    juce::AudioBuffer<float> tempBuffer;
    std::atomic<double> internalSampleRate { 44100.0 };
    std::atomic<int> lastMidiCount { 0 };
};
