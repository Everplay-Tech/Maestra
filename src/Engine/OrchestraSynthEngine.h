#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <array>
#include <cmath>

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

    // Global articulation definitions: indices 0..(numArticulations-1)
    static constexpr int numArticulations = 3;
    static constexpr int articulationKeyswitchBaseNote = 24; // C1..E1 map to articulations 0..2

    // Per-section, user-facing parameters
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

        int   maxVoices = 32;       // per-section allocation
        int   articulationIndex = 0; // current articulation 0..numArticulations-1
    };

    struct SectionStateSnapshot
    {
        SectionParams params;
        int activeVoices = 0;
    };

    OrchestraSynthEngine (PresetManager& presetManagerIn,
                          PerformanceMonitor& perfMonIn,
                          Logger& loggerIn)
        : presetManager (presetManagerIn),
          perfMon (perfMonIn),
          logger (loggerIn),
          convolutionReverb (loggerIn),
          oversampler (loggerIn)
    {
        // Distribute 176 voices across 5 sections: 48 + 4*32
        sectionParams[Strings].maxVoices    = 48;
        sectionParams[Brass].maxVoices      = 32;
        sectionParams[Woodwinds].maxVoices  = 32;
        sectionParams[Percussion].maxVoices = 32;
        sectionParams[Choir].maxVoices      = 32;

        initialiseArticulations();
    }

    OrchestraSynthEngine (const OrchestraSynthEngine&) = delete;
    OrchestraSynthEngine& operator= (const OrchestraSynthEngine&) = delete;
    OrchestraSynthEngine (OrchestraSynthEngine&&) = delete;
    OrchestraSynthEngine& operator= (OrchestraSynthEngine&&) = delete;

    // =========================================================
    // Public API used by standalone + plugin
    // =========================================================

    void prepare (double sampleRate, int samplesPerBlock)
    {
        // Prepare shared DSP
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
        spec.numChannels = 2;

        convolutionReverb.prepare (spec);
        oversampler.prepare (spec);

        internalSampleRate.store (sampleRate, std::memory_order_release);

        // Prepare per-section synthesisers and voices
        for (int sec = 0; sec < numSections; ++sec)
        {
            auto& runtime = sectionRuntime[sec];

            runtime.synth.clearVoices();
            runtime.synth.clearSounds();
            runtime.synth.setNoteStealingEnabled (true);
            runtime.synth.setCurrentPlaybackSampleRate (sampleRate);

            const auto voicesForSection = sectionParams[(SectionIndex)sec].maxVoices;

            for (int v = 0; v < voicesForSection; ++v)
            {
                runtime.synth.addVoice (new SectionVoice ((SectionIndex) sec,
                                                          sectionParams,
                                                          sectionRuntime));
            }

            runtime.synth.addSound (new SectionSound ((SectionIndex) sec));
        }

        lastBlockSize.store (samplesPerBlock, std::memory_order_release);
    }

    void reset()
    {
        convolutionReverb.reset();
        oversampler.reset();

        for (int sec = 0; sec < numSections; ++sec)
            sectionRuntime[sec].synth.allNotesOff (0, false);
    }

    // MIDI is routed deterministically to sections based on channel:
    //   ch 1 -> Strings, 2 -> Brass, 3 -> Woodwinds, 4 -> Percussion, 5 -> Choir
    // Articulation keyswitches: on each section's channel,
    //   note 24/25/26 select articulation 0/1/2 for that section.
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        const auto numSamples = buffer.getNumSamples();
        perfMon.beginBlock();

        splitMidiBySection (midi, numSamples);
        buffer.clear();

        // Oversampling wrapper and rendering
        oversampler.beginOversampledBlock (buffer);

        for (int sec = 0; sec < numSections; ++sec)
        {
            auto& runtime = sectionRuntime[sec];
            auto& secBuffer = buffer; // direct mix; each synth adds into buffer

            runtime.synth.renderNextBlock (secBuffer,
                                           sectionRuntime[sec].midiBuffer,
                                           0,
                                           numSamples);
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
        s.activeVoices = sectionRuntime[(int) index].synth.getNumActiveVoices();
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

    // PresetManager hooks
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

            sectionTree.setProperty (juce::Identifier ("maxVoices"),       p.maxVoices, nullptr);
            sectionTree.setProperty (juce::Identifier ("gain"),            p.gain, nullptr);
            sectionTree.setProperty (juce::Identifier ("pan"),             p.pan, nullptr);
            sectionTree.setProperty (juce::Identifier ("cutoff"),          p.cutoff, nullptr);
            sectionTree.setProperty (juce::Identifier ("resonance"),       p.resonance, nullptr);
            sectionTree.setProperty (juce::Identifier ("attackMs"),        p.attackMs, nullptr);
            sectionTree.setProperty (juce::Identifier ("releaseMs"),       p.releaseMs, nullptr);
            sectionTree.setProperty (juce::Identifier ("reverbSend"),      p.reverbSend, nullptr);
            sectionTree.setProperty (juce::Identifier ("oversampleFactor"),p.oversampleFactor, nullptr);
            sectionTree.setProperty (juce::Identifier ("articulationIndex"),p.articulationIndex, nullptr);

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
            p.maxVoices        = (int)   t.getProperty (juce::Identifier ("maxVoices"),        p.maxVoices);
            p.gain             = (float) t.getProperty (juce::Identifier ("gain"),             p.gain);
            p.pan              = (float) t.getProperty (juce::Identifier ("pan"),              p.pan);
            p.cutoff           = (float) t.getProperty (juce::Identifier ("cutoff"),           p.cutoff);
            p.resonance        = (float) t.getProperty (juce::Identifier ("resonance"),        p.resonance);
            p.attackMs         = (float) t.getProperty (juce::Identifier ("attackMs"),         p.attackMs);
            p.releaseMs        = (float) t.getProperty (juce::Identifier ("releaseMs"),        p.releaseMs);
            p.reverbSend       = (float) t.getProperty (juce::Identifier ("reverbSend"),       p.reverbSend);
            p.oversampleFactor = (float) t.getProperty (juce::Identifier ("oversampleFactor"), p.oversampleFactor);
            p.articulationIndex= (int)   t.getProperty (juce::Identifier ("articulationIndex"),p.articulationIndex);
        };

        loadSection (Strings,    "strings");
        loadSection (Brass,      "brass");
        loadSection (Woodwinds,  "woodwinds");
        loadSection (Percussion, "percussion");
        loadSection (Choir,      "choir");
    }

private:
    // =========================================================
    // Articulation model
    // =========================================================

    struct ArticulationParams
    {
        float attackMs = 5.0f;
        float decayMs  = 50.0f;
        float sustain  = 0.8f;
        float releaseMs= 200.0f;

        float filterCutoff   = 12000.0f;
        float filterResonance= 0.7f;
    };

    struct SectionRuntime
    {
        juce::Synthesiser synth;
        juce::MidiBuffer midiBuffer;
        std::array<ArticulationParams, numArticulations> articulations {};
        int currentArticulationIndex = 0;
    };

    // Nested JUCE sound describing one section
    class SectionSound : public juce::SynthesiserSound
    {
    public:
        explicit SectionSound (SectionIndex sec) : section (sec) {}

        bool appliesToNote (int) override               { return true; }
        bool appliesToChannel (int) override            { return true; }

        SectionIndex getSection() const noexcept        { return section; }

    private:
        SectionIndex section;
    };

    // Voice that reads SectionParams + Articulation per section
    class SectionVoice : public juce::SynthesiserVoice
    {
    public:
        SectionVoice (SectionIndex sectionIn,
                      std::array<SectionParams, numSections>& sectionParamsIn,
                      std::array<SectionRuntime, numSections>& sectionRuntimeIn)
            : section (sectionIn),
              sectionParams (sectionParamsIn),
              sectionRuntime (sectionRuntimeIn)
        {
            updateFilterSampleRate (44100.0);
        }

        bool canPlaySound (juce::SynthesiserSound* sound) override
        {
            if (auto* s = dynamic_cast<SectionSound*> (sound))
                return s->getSection() == section;
            return false;
        }

        void startNote (int midiNoteNumber,
                        float velocity,
                        juce::SynthesiserSound* /*sound*/,
                        int /*currentPitchWheelPosition*/) override
        {
            currentMidiNote = midiNoteNumber;
            currentVelocity = velocity;

            const auto& p = sectionParams[(int) section];
            const auto& art = getCurrentArticulation();

            juce::ADSR::Parameters adsrParams;
            adsrParams.attack  = art.attackMs  * 0.001f;
            adsrParams.decay   = art.decayMs   * 0.001f;
            adsrParams.sustain = art.sustain;
            adsrParams.release = art.releaseMs * 0.001f;
            adsr.setParameters (adsrParams);
            adsr.noteOn();

            filter.reset();
            setFilterParams (art.filterCutoff, art.filterResonance);

            level = p.gain * juce::jlimit (0.0f, 1.0f, velocity);
            updatePanGains (p.pan);
        }

        void stopNote (float /*velocity*/, bool allowTailOff) override
        {
            if (allowTailOff)
            {
                adsr.noteOff();
            }
            else
            {
                clearCurrentNote();
                adsr.reset();
            }
        }

        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}

        void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                              int startSample,
                              int numSamples) override
        {
            if (! isVoiceActive())
                return;

            tempBuffer.setSize (1, numSamples, false, false, true);
            tempBuffer.clear();

            auto* mono = tempBuffer.getWritePointer (0);

            const auto sampleRate = currentSampleRate > 0.0 ? currentSampleRate : 44100.0;
            const auto freq = juce::MidiMessage::getMidiNoteInHertz (currentMidiNote);

            // Simple band-limited-ish waveform: sum of two detuned sines, then filtered
            for (int n = 0; n < numSamples; ++n)
            {
                const double t = phase / sampleRate;
                const double x1 = std::sin (juce::MathConstants<double>::twoPi * freq * t);
                const double x2 = std::sin (juce::MathConstants<double>::twoPi * (freq * 1.01) * t);
                mono[n] = (float) (0.5 * (x1 + x2));
                phase += 1.0;
            }

            juce::dsp::AudioBlock<float> block (tempBuffer);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            filter.process (ctx);

            adsr.applyEnvelopeToBuffer (tempBuffer, 0, numSamples);

            if (! adsr.isActive())
            {
                clearCurrentNote();
                return;
            }

            auto* left  = outputBuffer.getWritePointer (0, startSample);
            auto* right = outputBuffer.getNumChannels() > 1
                          ? outputBuffer.getWritePointer (1, startSample)
                          : nullptr;

            for (int n = 0; n < numSamples; ++n)
            {
                const float s = mono[n] * level;
                left[n]  += s * panLeft;
                if (right != nullptr)
                    right[n] += s * panRight;
            }
        }

        void setCurrentPlaybackSampleRate (double newRate) override
        {
            currentSampleRate = newRate;
            updateFilterSampleRate (newRate);
        }

    private:
        ArticulationParams getCurrentArticulation() const
        {
            const auto secIdx = (int) section;
            const auto idx = juce::jlimit (0, numArticulations - 1,
                                           sectionRuntime[secIdx].currentArticulationIndex);
            return sectionRuntime[secIdx].articulations[(size_t) idx];
        }

        void setFilterParams (float cutoff, float resonance)
        {
            auto& f = filter.state->parameters;
            f.type = juce::dsp::StateVariableTPTFilterType::lowpass;
            f.setCutOffFrequency (currentSampleRate, cutoff, resonance);
        }

        void updateFilterSampleRate (double newRate)
        {
            juce::dsp::ProcessSpec spec;
            spec.sampleRate = newRate > 0.0 ? newRate : 44100.0;
            spec.maximumBlockSize = 512;
            spec.numChannels = 1;
            filter.prepare (spec);
        }

        void updatePanGains (float pan)
        {
            const auto p = juce::jlimit (-1.0f, 1.0f, pan);
            const auto angle = (p + 1.0f) * juce::MathConstants<float>::halfPi * 0.5f;
            panLeft  = std::cos (angle);
            panRight = std::sin (angle);
        }

        SectionIndex section;
        std::array<SectionParams, numSections>& sectionParams;
        std::array<SectionRuntime, numSections>& sectionRuntime;

        int   currentMidiNote = 60;
        float currentVelocity = 1.0f;
        double currentSampleRate = 44100.0;
        double phase = 0.0;

        float level = 0.0f;
        float panLeft = 1.0f;
        float panRight = 1.0f;

        juce::ADSR adsr;
        juce::dsp::StateVariableTPTFilter<float> filter;
        juce::AudioBuffer<float> tempBuffer;
    };

    void initialiseArticulations()
    {
        auto setArt = [this](SectionIndex sec, int idx,
                             float aMs, float dMs, float sust, float rMs,
                             float cutoff, float q)
        {
            auto& art = sectionRuntime[(int) sec].articulations[(size_t) idx];
            art.attackMs        = aMs;
            art.decayMs         = dMs;
            art.sustain         = sust;
            art.releaseMs       = rMs;
            art.filterCutoff    = cutoff;
            art.filterResonance = q;
        };

        // 0 = sustain, 1 = staccato, 2 = legato-ish
        for (int sec = 0; sec < numSections; ++sec)
        {
            auto s = static_cast<SectionIndex> (sec);
            setArt (s, 0, 10.0f, 60.0f, 0.9f, 250.0f, 12000.0f, 0.7f);  // sustain
            setArt (s, 1, 2.0f,  15.0f, 0.6f, 80.0f,  8000.0f, 0.9f);   // staccato
            setArt (s, 2, 30.0f, 80.0f, 0.95f, 400.0f, 10000.0f, 0.6f); // legato
        }
    }

    void splitMidiBySection (juce::MidiBuffer& midi, int /*numSamples*/)
    {
        for (int sec = 0; sec < numSections; ++sec)
            sectionRuntime[sec].midiBuffer.clear();

        int eventCount = 0;

        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            const auto pos = metadata.samplePosition;
            ++eventCount;

            const int channel = msg.getChannel();
            const int sectionIndex = channel - 1; // ch1..5 -> 0..4

            if (sectionIndex < 0 || sectionIndex >= numSections)
                continue;

            auto& runtime = sectionRuntime[sectionIndex];

            // Articulation keyswitch handling
            if (msg.isNoteOn())
            {
                const int note = msg.getNoteNumber();
                const int relative = note - articulationKeyswitchBaseNote;

                if (relative >= 0 && relative < numArticulations)
                {
                    runtime.currentArticulationIndex = relative;
                    continue; // swallow keyswitch events
                }
            }

            runtime.midiBuffer.addEvent (msg, pos);
        }

        lastMidiCount.store (eventCount, std::memory_order_relaxed);
        midi.clear(); // consumed into per-section buffers
    }

    // =========================================================
    // Members
    // =========================================================

    PresetManager& presetManager;
    PerformanceMonitor& perfMon;
    Logger& logger;

    ConvolutionEngine convolutionReverb;
    Oversampler oversampler;
    ImpulseResponseLoader irLoader;

    std::array<SectionParams, numSections> sectionParams {};
    std::array<SectionRuntime, numSections> sectionRuntime {};

    std::atomic<double> internalSampleRate { 44100.0 };
    std::atomic<int> lastBlockSize { 512 };
    std::atomic<int> lastMidiCount { 0 };
};
