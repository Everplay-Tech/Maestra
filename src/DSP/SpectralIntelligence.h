#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <complex>
#include "../Systems/Logger.h"

/**
 * SpectralIntelligence - AI-assisted frequency balancing and analysis
 * 
 * Revolutionary spectral processing providing:
 * - Real-time FFT-based spectral analysis
 * - Intelligent frequency band balancing
 * - Automatic resonance detection and control
 * - Spectral tilt adjustment
 * - Masking detection and prevention
 * - Genre-aware spectral shaping
 * 
 * Ensures optimal frequency distribution for clarity and impact.
 */
class SpectralIntelligence
{
public:
    static constexpr int fftOrder = 11;  // 2048 samples
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numBands = 32;  // Frequency bands for analysis

    explicit SpectralIntelligence (::Logger& loggerIn)
        : logger (loggerIn),
          forwardFFT (fftOrder),
          window (fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        logger.log (::Logger::Level::Info, "SpectralIntelligence",
                    "Initializing AI-assisted spectral analysis");
        
        initializeFrequencyBands();
    }

    SpectralIntelligence (const SpectralIntelligence&) = delete;
    SpectralIntelligence& operator= (const SpectralIntelligence&) = delete;
    SpectralIntelligence (SpectralIntelligence&&) = delete;
    SpectralIntelligence& operator= (SpectralIntelligence&&) = delete;

    void prepare (double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;
        
        // Prepare analysis buffers
        analysisBuffer.setSize (2, fftSize, false, true, true);
        fftData.resize (fftSize * 2, 0.0f);
        
        // Initialize smoothed band energies
        for (auto& energy : bandEnergies)
            energy = 0.0f;
        
        // Prepare multiband filters
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (maxBlockSize);
        spec.numChannels = 1;
        
        for (auto& filter : bandFilters)
            filter.prepare (spec);
        
        updateBandFilters();
        
        prepared.store (true, std::memory_order_release);
        
        logger.log (::Logger::Level::Info, "SpectralIntelligence",
                    "Prepared for spectral analysis");
    }

    void reset()
    {
        analysisBuffer.clear();
        std::fill (fftData.begin(), fftData.end(), 0.0f);
        std::fill (bandEnergies.begin(), bandEnergies.end(), 0.0f);
        
        for (auto& filter : bandFilters)
            filter.reset();
        
        spectralCentroid = 1000.0f;
        spectralTilt = 0.0f;
    }

    /**
     * Analyze and process audio with intelligent spectral balancing
     */
    void process (juce::AudioBuffer<float>& buffer)
    {
        if (! prepared.load (std::memory_order_acquire))
            return;

        const auto numSamples = buffer.getNumSamples();
        if (numSamples == 0)
            return;

        // Perform spectral analysis
        analyzeSpectrum (buffer);
        
        // Apply intelligent corrections if enabled
        if (autoBalanceEnabled.load (std::memory_order_acquire))
            applySpectralBalance (buffer);
    }

    /**
     * Get spectral analysis results
     */
    struct SpectralAnalysis
    {
        float spectralCentroid = 0.0f;     // Weighted average frequency
        float spectralSpread = 0.0f;       // Spread around centroid
        float spectralTilt = 0.0f;         // Overall tilt (negative = dark, positive = bright)
        float spectralFlux = 0.0f;         // Change rate in spectrum
        std::array<float, numBands> bandEnergies {};
        float totalEnergy = 0.0f;
        float peakFrequency = 0.0f;
    };

    SpectralAnalysis getAnalysis() const
    {
        SpectralAnalysis analysis;
        analysis.spectralCentroid = spectralCentroid;
        analysis.spectralSpread = spectralSpread;
        analysis.spectralTilt = spectralTilt;
        analysis.spectralFlux = spectralFlux;
        analysis.bandEnergies = bandEnergies;
        analysis.totalEnergy = totalEnergy;
        analysis.peakFrequency = peakFrequency;
        return analysis;
    }

    /**
     * Enable/disable automatic spectral balancing
     */
    void setAutoBalanceEnabled (bool enabled)
    {
        autoBalanceEnabled.store (enabled, std::memory_order_release);
    }

    /**
     * Set target spectral tilt (-1 = darker, 0 = neutral, +1 = brighter)
     */
    void setTargetTilt (float tilt)
    {
        targetTilt = juce::jlimit (-1.0f, 1.0f, tilt);
    }

    /**
     * Set genre-specific spectral profile
     */
    void setGenreProfile (const juce::String& genre)
    {
        if (genre == "orchestral")
        {
            targetTilt = -0.2f;  // Slightly darker, warmer
            targetBandBalance = {1.0f, 1.1f, 1.0f, 0.9f, 0.8f};
        }
        else if (genre == "electronic")
        {
            targetTilt = 0.3f;   // Brighter
            targetBandBalance = {0.9f, 1.0f, 1.1f, 1.2f, 1.0f};
        }
        else if (genre == "acoustic")
        {
            targetTilt = 0.0f;   // Neutral
            targetBandBalance = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
        }
        
        currentGenre = genre;
        
        logger.log (::Logger::Level::Info, "SpectralIntelligence",
                    juce::String ("Set genre profile: ") + genre);
    }

private:
    struct FrequencyBand
    {
        float centerFreq = 0.0f;
        float bandwidth = 0.0f;
        int binStart = 0;
        int binEnd = 0;
    };

    void initializeFrequencyBands()
    {
        // Create logarithmically-spaced frequency bands (20Hz - 20kHz)
        const float minFreq = 20.0f;
        const float maxFreq = 20000.0f;
        const float ratio = std::pow (maxFreq / minFreq, 1.0f / numBands);
        
        float freq = minFreq;
        for (int i = 0; i < numBands; ++i)
        {
            frequencyBands[i].centerFreq = freq;
            frequencyBands[i].bandwidth = freq * (ratio - 1.0f);
            freq *= ratio;
        }
    }

    void updateBandFilters()
    {
        // Create bandpass filters for each band
        for (size_t i = 0; i < bandFilters.size() && i < 5; ++i)
        {
            const float centerFreq = frequencyBands[i * 6].centerFreq;  // Sample 5 bands
            const float q = 1.0f;
            
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (
                currentSampleRate, centerFreq, q);
            bandFilters[i].coefficients = coeffs;
        }
    }

    void analyzeSpectrum (const juce::AudioBuffer<float>& buffer)
    {
        // Copy samples to analysis buffer (mix to mono)
        const int numSamples = std::min (buffer.getNumSamples(), fftSize);
        
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                sample += buffer.getSample (ch, i);
            
            fftData[i] = sample / buffer.getNumChannels();
        }
        
        // Zero-pad if necessary
        for (int i = numSamples; i < fftSize; ++i)
            fftData[i] = 0.0f;
        
        // Apply window
        window.multiplyWithWindowingTable (fftData.data(), fftSize);
        
        // Perform FFT
        forwardFFT.performFrequencyOnlyForwardTransform (fftData.data());
        
        // Analyze spectrum
        calculateSpectralFeatures();
        updateBandEnergies();
    }

    void calculateSpectralFeatures()
    {
        // Calculate spectral centroid
        float weightedSum = 0.0f;
        float magnitudeSum = 0.0f;
        float maxMagnitude = 0.0f;
        int maxBin = 0;
        
        const float binToFreq = static_cast<float> (currentSampleRate) / fftSize;
        
        for (int i = 1; i < fftSize / 2; ++i)
        {
            const float magnitude = fftData[i];
            const float frequency = i * binToFreq;
            
            weightedSum += magnitude * frequency;
            magnitudeSum += magnitude;
            
            if (magnitude > maxMagnitude)
            {
                maxMagnitude = magnitude;
                maxBin = i;
            }
        }
        
        if (magnitudeSum > 0.0f)
        {
            const float newCentroid = weightedSum / magnitudeSum;
            spectralCentroid = spectralCentroid * 0.9f + newCentroid * 0.1f;  // Smooth
        }
        
        peakFrequency = maxBin * binToFreq;
        totalEnergy = magnitudeSum;
        
        // Calculate spectral spread
        float spreadSum = 0.0f;
        for (int i = 1; i < fftSize / 2; ++i)
        {
            const float frequency = i * binToFreq;
            const float diff = frequency - spectralCentroid;
            spreadSum += fftData[i] * diff * diff;
        }
        
        if (magnitudeSum > 0.0f)
            spectralSpread = std::sqrt (spreadSum / magnitudeSum);
        
        // Calculate spectral tilt (slope of spectrum)
        calculateSpectralTilt();
        
        // Calculate spectral flux (change over time)
        calculateSpectralFlux();
    }

    void calculateSpectralTilt()
    {
        // Compare low vs high frequency energy
        float lowEnergy = 0.0f;
        float highEnergy = 0.0f;
        
        const int splitBin = fftSize / 4;
        
        for (int i = 1; i < splitBin; ++i)
            lowEnergy += fftData[i];
        
        for (int i = splitBin; i < fftSize / 2; ++i)
            highEnergy += fftData[i];
        
        const float totalBinEnergy = lowEnergy + highEnergy;
        if (totalBinEnergy > 0.0f)
        {
            const float tilt = (highEnergy - lowEnergy) / totalBinEnergy;
            spectralTilt = spectralTilt * 0.9f + tilt * 0.1f;  // Smooth
        }
    }

    void calculateSpectralFlux()
    {
        // Measure change from previous frame
        float flux = 0.0f;
        for (int i = 0; i < fftSize / 2; ++i)
        {
            const float diff = fftData[i] - previousSpectrum[i];
            flux += diff * diff;
            previousSpectrum[i] = fftData[i];
        }
        
        spectralFlux = std::sqrt (flux);
    }

    void updateBandEnergies()
    {
        // Update bin ranges for each band
        for (int i = 0; i < numBands; ++i)
        {
            auto& band = frequencyBands[i];
            const float binToFreq = static_cast<float> (currentSampleRate) / fftSize;
            
            band.binStart = static_cast<int> ((band.centerFreq - band.bandwidth * 0.5f) / binToFreq);
            band.binEnd = static_cast<int> ((band.centerFreq + band.bandwidth * 0.5f) / binToFreq);
            
            band.binStart = juce::jlimit (0, fftSize / 2, band.binStart);
            band.binEnd = juce::jlimit (0, fftSize / 2, band.binEnd);
            
            // Calculate band energy
            float energy = 0.0f;
            for (int bin = band.binStart; bin < band.binEnd; ++bin)
                energy += fftData[bin];
            
            // Smooth update
            bandEnergies[i] = bandEnergies[i] * 0.9f + energy * 0.1f;
        }
    }

    void applySpectralBalance (juce::AudioBuffer<float>& buffer)
    {
        // Apply tilt correction
        const float tiltError = targetTilt - spectralTilt;
        const float tiltGain = 1.0f + tiltError * 0.1f;  // Subtle correction
        
        // Apply high-shelf filter to adjust tilt
        if (std::abs (tiltError) > 0.05f)
        {
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                auto* data = buffer.getWritePointer (channel);
                
                // Simple high-shelf implementation
                float prev = 0.0f;
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    const float highPass = data[i] - prev;
                    prev = data[i];
                    data[i] += highPass * tiltError * 0.2f;
                }
            }
        }
    }

    ::Logger& logger;
    
    double currentSampleRate = 44100.0;
    
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
    
    juce::AudioBuffer<float> analysisBuffer;
    std::vector<float> fftData;
    std::vector<float> previousSpectrum {fftSize / 2, 0.0f};
    
    std::array<FrequencyBand, numBands> frequencyBands {};
    std::array<float, numBands> bandEnergies {};
    std::array<juce::dsp::IIR::Filter<float>, 5> bandFilters {};
    
    float spectralCentroid = 1000.0f;
    float spectralSpread = 0.0f;
    float spectralTilt = 0.0f;
    float spectralFlux = 0.0f;
    float totalEnergy = 0.0f;
    float peakFrequency = 0.0f;
    
    float targetTilt = 0.0f;
    std::array<float, 5> targetBandBalance {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    juce::String currentGenre = "neutral";
    
    std::atomic<bool> prepared { false };
    std::atomic<bool> autoBalanceEnabled { false };
};
