#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "../Systems/Logger.h"

/**
 * HarmonicEnhancer - Intelligent harmonic series generation and enhancement
 * 
 * Revolutionary harmonic processing providing:
 * - Missing fundamental synthesis
 * - Even/odd harmonic emphasis
 * - Intelligent harmonic series generation
 * - Subharmonic synthesis
 * - Harmonic exciter for "air" and "presence"
 * - Musical harmonic relationships
 * 
 * Enhances timbral richness and perceived loudness while
 * maintaining natural, musical characteristics.
 */
class HarmonicEnhancer
{
public:
    static constexpr int maxHarmonics = 16;

    explicit HarmonicEnhancer (::Logger& loggerIn)
        : logger (loggerIn)
    {
        logger.log (::Logger::Level::Info, "HarmonicEnhancer",
                    "Initializing intelligent harmonic enhancement");
    }

    HarmonicEnhancer (const HarmonicEnhancer&) = delete;
    HarmonicEnhancer& operator= (const HarmonicEnhancer&) = delete;
    HarmonicEnhancer (HarmonicEnhancer&&) = delete;
    HarmonicEnhancer& operator= (HarmonicEnhancer&&) = delete;

    void prepare (double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;
        
        // Prepare harmonics buffer
        harmonicsBuffer.setSize (2, maxBlockSize, false, true, true);
        
        // Prepare tracking filters for fundamental detection
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (maxBlockSize);
        spec.numChannels = 1;
        
        for (auto& filter : trackingFilters)
        {
            filter.prepare (spec);
            filter.reset();
        }
        
        // Initialize harmonic generators
        for (auto& gen : harmonicGenerators)
        {
            gen.phase = 0.0;
            gen.amplitude = 0.0f;
        }
        
        prepared.store (true, std::memory_order_release);
        
        logger.log (::Logger::Level::Info, "HarmonicEnhancer",
                    "Prepared for harmonic enhancement");
    }

    void reset()
    {
        harmonicsBuffer.clear();
        
        for (auto& filter : trackingFilters)
            filter.reset();
        
        for (auto& gen : harmonicGenerators)
        {
            gen.phase = 0.0;
            gen.amplitude = 0.0f;
        }
        
        detectedFundamental = 0.0f;
    }

    /**
     * Process audio with intelligent harmonic enhancement
     * @param buffer Audio to enhance
     * @param fundamentalHint Optional fundamental frequency hint (0 = auto-detect)
     */
    void process (juce::AudioBuffer<float>& buffer, float fundamentalHint = 0.0f)
    {
        if (! prepared.load (std::memory_order_acquire))
            return;

        const auto numSamples = buffer.getNumSamples();
        if (numSamples == 0)
            return;

        // Detect or use provided fundamental
        float fundamental = fundamentalHint;
        if (fundamental <= 0.0f)
            fundamental = detectFundamental (buffer);
        else
            detectedFundamental = fundamental;
        
        if (fundamental < 20.0f || fundamental > 8000.0f)
            return;  // Out of useful range
        
        // Generate harmonics
        generateHarmonics (fundamental, numSamples);
        
        // Mix harmonics into original signal
        mixHarmonics (buffer);
    }

    /**
     * Set enhancement mode
     */
    enum class EnhancementMode
    {
        Warmth,         // Emphasize low harmonics, add subharmonics
        Brightness,     // Emphasize high harmonics, add "air"
        Presence,       // Boost mid-high harmonics for clarity
        Fullness,       // Balanced across spectrum
        Vintage,        // Even harmonics (tube-like)
        Modern          // Crisp, extended highs
    };

    void setEnhancementMode (EnhancementMode mode)
    {
        currentMode = mode;
        updateHarmonicWeights();
        
        const char* modeNames[] = {"Warmth", "Brightness", "Presence", "Fullness", "Vintage", "Modern"};
        logger.log (::Logger::Level::Info, "HarmonicEnhancer",
                    juce::String ("Set mode: ") + modeNames[static_cast<int> (mode)]);
    }

    /**
     * Set overall enhancement amount (0-1)
     */
    void setAmount (float amount)
    {
        enhancementAmount = juce::jlimit (0.0f, 1.0f, amount);
    }

    /**
     * Set mix balance (0 = dry, 1 = 100% harmonics)
     */
    void setMix (float mix)
    {
        harmonicsMix = juce::jlimit (0.0f, 1.0f, mix);
    }

    /**
     * Enable/disable subharmonic synthesis
     */
    void setSubharmonicsEnabled (bool enabled)
    {
        subharmonicsEnabled.store (enabled, std::memory_order_release);
    }

    struct EnhancerState
    {
        float detectedFundamental = 0.0f;
        float totalHarmonicEnergy = 0.0f;
        int activeHarmonics = 0;
        float enhancementAmount = 0.0f;
    };

    EnhancerState getState() const
    {
        EnhancerState state;
        state.detectedFundamental = detectedFundamental;
        state.totalHarmonicEnergy = totalHarmonicEnergy;
        state.activeHarmonics = activeHarmonics;
        state.enhancementAmount = enhancementAmount;
        return state;
    }

private:
    struct HarmonicGenerator
    {
        double phase = 0.0;
        float amplitude = 0.0f;
        int harmonicNumber = 1;  // 1 = fundamental, 2 = 2nd harmonic, etc.
    };

    float detectFundamental (const juce::AudioBuffer<float>& buffer)
    {
        // Simple autocorrelation-based pitch detection
        // In production, use more robust algorithms (YIN, SWIPE, etc.)
        
        const int analysisLength = std::min (2048, buffer.getNumSamples());
        const int maxLag = static_cast<int> (currentSampleRate / 50.0f);  // 50 Hz minimum
        
        float maxCorrelation = 0.0f;
        int bestLag = 0;
        
        // Mix channels to mono for analysis
        std::vector<float> mono (analysisLength);
        for (int i = 0; i < analysisLength; ++i)
        {
            float sample = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                sample += buffer.getSample (ch, i);
            mono[i] = sample / buffer.getNumChannels();
        }
        
        // Autocorrelation
        for (int lag = 1; lag < maxLag; ++lag)
        {
            float correlation = 0.0f;
            for (int i = 0; i < analysisLength - lag; ++i)
                correlation += mono[i] * mono[i + lag];
            
            if (correlation > maxCorrelation)
            {
                maxCorrelation = correlation;
                bestLag = lag;
            }
        }
        
        if (bestLag > 0)
        {
            const float detected = static_cast<float> (currentSampleRate) / bestLag;
            
            // Smooth detection
            detectedFundamental = detectedFundamental * 0.9f + detected * 0.1f;
        }
        
        return detectedFundamental;
    }

    void generateHarmonics (float fundamental, int numSamples)
    {
        harmonicsBuffer.clear();
        
        activeHarmonics = 0;
        totalHarmonicEnergy = 0.0f;
        
        const double phaseIncrement = juce::MathConstants<double>::twoPi * fundamental / currentSampleRate;
        
        // Generate each harmonic
        for (int h = 0; h < maxHarmonics; ++h)
        {
            auto& gen = harmonicGenerators[h];
            gen.harmonicNumber = h + 1;
            
            // Skip if frequency too high
            if (fundamental * gen.harmonicNumber > currentSampleRate * 0.4f)
                continue;
            
            // Get weight for this harmonic based on mode
            const float weight = harmonicWeights[h];
            if (weight < 0.001f)
                continue;
            
            gen.amplitude = weight * enhancementAmount;
            activeHarmonics++;
            
            // Generate samples
            for (int channel = 0; channel < harmonicsBuffer.getNumChannels(); ++channel)
            {
                auto* data = harmonicsBuffer.getWritePointer (channel);
                double phase = gen.phase;
                
                for (int i = 0; i < numSamples; ++i)
                {
                    const float sample = static_cast<float> (std::sin (phase * gen.harmonicNumber));
                    data[i] += sample * gen.amplitude;
                    
                    phase += phaseIncrement;
                    if (phase >= juce::MathConstants<double>::twoPi)
                        phase -= juce::MathConstants<double>::twoPi;
                }
                
                if (channel == 0)
                    gen.phase = phase;
            }
            
            totalHarmonicEnergy += gen.amplitude;
        }
        
        // Generate subharmonics if enabled
        if (subharmonicsEnabled.load (std::memory_order_acquire))
            generateSubharmonics (fundamental, numSamples);
    }

    void generateSubharmonics (float fundamental, int numSamples)
    {
        // Add octave down and fifth down for fullness
        const std::array<float, 2> subharmonicRatios = {0.5f, 0.3333f};  // Octave, fifth
        
        for (float ratio : subharmonicRatios)
        {
            const float subFreq = fundamental * ratio;
            if (subFreq < 20.0f)
                continue;
            
            const double phaseInc = juce::MathConstants<double>::twoPi * subFreq / currentSampleRate;
            
            for (int channel = 0; channel < harmonicsBuffer.getNumChannels(); ++channel)
            {
                auto* data = harmonicsBuffer.getWritePointer (channel);
                
                for (int i = 0; i < numSamples; ++i)
                {
                    const float sample = std::sin (subharmonicPhase + phaseInc * i);
                    data[i] += sample * enhancementAmount * 0.3f;
                }
            }
            
            subharmonicPhase += phaseInc * numSamples;
            while (subharmonicPhase >= juce::MathConstants<double>::twoPi)
                subharmonicPhase -= juce::MathConstants<double>::twoPi;
        }
    }

    void mixHarmonics (juce::AudioBuffer<float>& buffer)
    {
        const auto numSamples = buffer.getNumSamples();
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* dry = buffer.getWritePointer (channel);
            const auto* wet = harmonicsBuffer.getReadPointer (channel % harmonicsBuffer.getNumChannels());
            
            for (int i = 0; i < numSamples; ++i)
            {
                // Parallel mix with soft saturation on harmonics
                const float harmonic = std::tanh (wet[i] * 2.0f) * 0.5f;
                dry[i] = dry[i] * (1.0f - harmonicsMix) + harmonic * harmonicsMix;
            }
        }
    }

    void updateHarmonicWeights()
    {
        // Set weights based on enhancement mode
        switch (currentMode)
        {
            case EnhancementMode::Warmth:
                // Emphasize low harmonics
                for (int i = 0; i < maxHarmonics; ++i)
                    harmonicWeights[i] = 1.0f / (i + 1.0f);
                break;
                
            case EnhancementMode::Brightness:
                // Emphasize high harmonics
                for (int i = 0; i < maxHarmonics; ++i)
                    harmonicWeights[i] = (i + 1.0f) / maxHarmonics;
                break;
                
            case EnhancementMode::Presence:
                // Boost mid-high range (harmonics 3-8)
                for (int i = 0; i < maxHarmonics; ++i)
                {
                    if (i >= 2 && i <= 7)
                        harmonicWeights[i] = 1.0f;
                    else
                        harmonicWeights[i] = 0.3f;
                }
                break;
                
            case EnhancementMode::Fullness:
                // Balanced
                for (int i = 0; i < maxHarmonics; ++i)
                    harmonicWeights[i] = 0.8f;
                break;
                
            case EnhancementMode::Vintage:
                // Even harmonics only (tube-like)
                for (int i = 0; i < maxHarmonics; ++i)
                    harmonicWeights[i] = (i % 2 == 1) ? 0.9f : 0.1f;  // i+1 is harmonic number
                break;
                
            case EnhancementMode::Modern:
                // Extended highs with controlled lows
                for (int i = 0; i < maxHarmonics; ++i)
                    harmonicWeights[i] = 0.5f + (i / (float)maxHarmonics) * 0.5f;
                break;
        }
    }

    ::Logger& logger;
    
    double currentSampleRate = 44100.0;
    
    juce::AudioBuffer<float> harmonicsBuffer;
    
    std::array<juce::dsp::IIR::Filter<float>, 4> trackingFilters {};
    std::array<HarmonicGenerator, maxHarmonics> harmonicGenerators {};
    std::array<float, maxHarmonics> harmonicWeights {};
    
    EnhancementMode currentMode = EnhancementMode::Fullness;
    
    float detectedFundamental = 0.0f;
    float enhancementAmount = 0.3f;
    float harmonicsMix = 0.2f;
    float totalHarmonicEnergy = 0.0f;
    int activeHarmonics = 0;
    
    double subharmonicPhase = 0.0;
    
    std::atomic<bool> prepared { false };
    std::atomic<bool> subharmonicsEnabled { false };
};
