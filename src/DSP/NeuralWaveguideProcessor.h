#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include "../Systems/Logger.h"

/**
 * NeuralWaveguideProcessor - Physical modeling meets neural networks
 * 
 * Revolutionary synthesis combining:
 * - Karplus-Strong extended waveguide synthesis
 * - Neural network-inspired adaptive coupling
 * - Self-organizing harmonic structure
 * - Dynamic dispersion control
 * 
 * This processor creates organic, evolving timbres that respond
 * intelligently to playing dynamics and musical context.
 */
class NeuralWaveguideProcessor
{
public:
    explicit NeuralWaveguideProcessor (::Logger& loggerIn)
        : logger (loggerIn)
    {
        logger.log (::Logger::Level::Info, "NeuralWaveguideProcessor",
                    "Initializing neural-waveguide hybrid processor");
        initializeNeuralCouplings();
    }

    NeuralWaveguideProcessor (const NeuralWaveguideProcessor&) = delete;
    NeuralWaveguideProcessor& operator= (const NeuralWaveguideProcessor&) = delete;
    NeuralWaveguideProcessor (NeuralWaveguideProcessor&&) = delete;
    NeuralWaveguideProcessor& operator= (NeuralWaveguideProcessor&&) = delete;

    void prepare (double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;
        
        // Prepare waveguide network
        for (auto& node : neuralNodes)
        {
            node.delayLine.resize (static_cast<size_t> (sampleRate * 0.1), 0.0f);  // 100ms max delay
            node.writePos = 0;
            node.energy = 0.0f;
        }
        
        // Prepare dispersion filters
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (maxBlockSize);
        spec.numChannels = 1;
        
        for (auto& filter : dispersionFilters)
            filter.prepare (spec);
        
        tempBuffer.setSize (1, maxBlockSize, false, true, true);
        
        prepared.store (true, std::memory_order_release);
        
        logger.log (::Logger::Level::Info, "NeuralWaveguideProcessor",
                    "Prepared neural waveguide network");
    }

    void reset()
    {
        for (auto& node : neuralNodes)
        {
            std::fill (node.delayLine.begin(), node.delayLine.end(), 0.0f);
            node.writePos = 0;
            node.energy = 0.0f;
            node.activation = 0.0f;
        }
        
        for (auto& filter : dispersionFilters)
            filter.reset();
        
        excitationEnergy = 0.0f;
    }

    /**
     * Excite the waveguide network with an impulse
     * @param frequency Target frequency in Hz
     * @param velocity Playing velocity (0-1)
     * @param brightness Timbral brightness (0-1)
     */
    void excite (float frequency, float velocity, float brightness)
    {
        if (! prepared.load (std::memory_order_acquire) || frequency <= 0.0f)
            return;

        const float energy = velocity * velocity;  // Quadratic response for dynamics
        excitationEnergy = energy;
        
        // Calculate optimal delay length for target frequency
        const float delayLength = static_cast<float> (currentSampleRate) / frequency;
        
        // Excite all neural nodes with phase-distributed impulses
        for (size_t i = 0; i < neuralNodes.size(); ++i)
        {
            auto& node = neuralNodes[i];
            
            // Calculate node-specific delay length (slight detuning for richness)
            const float detune = 1.0f + (i * 0.001f - 0.002f);
            node.targetDelayLength = delayLength * detune;
            node.currentDelayLength = node.targetDelayLength;
            
            // Set energy level with distribution
            node.energy = energy * (1.0f - i * 0.05f);
            
            // Inject excitation pulse into delay line
            const int pulseLength = std::min (64, static_cast<int> (node.delayLine.size()));
            for (int p = 0; p < pulseLength; ++p)
            {
                const float phase = static_cast<float> (p) / pulseLength;
                const float envelope = std::sin (phase * juce::MathConstants<float>::pi);
                
                // Shaped noise burst with brightness control
                const float noise = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
                const float harmonic = std::sin (juce::MathConstants<float>::twoPi * phase * brightness * 8.0f);
                
                node.delayLine[(node.writePos + p) % node.delayLine.size()] = 
                    (noise * 0.5f + harmonic * 0.5f) * envelope * energy;
            }
        }
        
        // Set dispersion based on brightness
        updateDispersion (brightness);
    }

    /**
     * Process the waveguide network and generate output
     */
    void process (juce::AudioBuffer<float>& buffer)
    {
        if (! prepared.load (std::memory_order_acquire))
            return;

        const auto numSamples = buffer.getNumSamples();
        if (numSamples == 0)
            return;

        tempBuffer.setSize (1, numSamples, false, false, true);
        tempBuffer.clear();
        
        auto* output = tempBuffer.getWritePointer (0);
        
        // Process each sample through the neural waveguide network
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = 0.0f;
            
            // Update all neural nodes
            for (size_t n = 0; n < neuralNodes.size(); ++n)
            {
                auto& node = neuralNodes[n];
                
                if (node.energy < 0.0001f)
                    continue;
                
                // Read from delay line with fractional delay interpolation
                const float readPos = node.writePos - node.currentDelayLength;
                const int pos1 = static_cast<int> (std::floor (readPos)) & (node.delayLine.size() - 1);
                const int pos2 = (pos1 + 1) & (node.delayLine.size() - 1);
                const float frac = readPos - std::floor (readPos);
                
                const float delayed = node.delayLine[pos1] * (1.0f - frac) + 
                                      node.delayLine[pos2] * frac;
                
                // Neural activation function (soft clipping)
                node.activation = std::tanh (delayed * 2.0f) * 0.5f;
                
                // Collect input from coupled nodes
                float coupledInput = 0.0f;
                for (size_t m = 0; m < neuralNodes.size(); ++m)
                {
                    if (m != n)
                    {
                        const float weight = neuralCouplings[n][m];
                        coupledInput += neuralNodes[m].activation * weight;
                    }
                }
                
                // Feedback with coupling, damping, and nonlinearity
                const float damping = 0.9995f - node.energy * 0.0005f;  // Energy-dependent damping
                const float nonlinear = delayed * delayed * 0.1f;  // Subtle nonlinearity
                const float feedback = delayed * damping - nonlinear + coupledInput * 0.1f;
                
                // Write back to delay line
                node.delayLine[node.writePos] = feedback;
                node.writePos = (node.writePos + 1) % node.delayLine.size();
                
                // Accumulate output
                sample += node.activation * node.energy;
                
                // Energy decay
                node.energy *= 0.99999f;
            }
            
            output[i] = sample * 0.25f;  // Scale output
        }
        
        // Apply dispersion filtering
        applyDispersion (tempBuffer);
        
        // Mix into output buffer (stereo)
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            const auto* sourceData = tempBuffer.getReadPointer (0);
            
            // Stereo width effect
            const float pan = (channel == 0) ? 0.9f : 1.1f;
            
            for (int i = 0; i < numSamples; ++i)
                channelData[i] += sourceData[i] * pan;
        }
        
        // Update neural couplings based on activity
        updateNeuralCouplings();
    }

    struct ProcessorState
    {
        float totalEnergy = 0.0f;
        float averageActivation = 0.0f;
        float couplingStrength = 0.0f;
        int activeNodes = 0;
    };

    ProcessorState getState() const
    {
        ProcessorState state;
        
        for (const auto& node : neuralNodes)
        {
            state.totalEnergy += node.energy;
            state.averageActivation += std::abs (node.activation);
            if (node.energy > 0.001f)
                ++state.activeNodes;
        }
        
        if (neuralNodes.size() > 0)
            state.averageActivation /= static_cast<float> (neuralNodes.size());
        
        // Calculate average coupling strength
        for (size_t i = 0; i < neuralNodes.size(); ++i)
            for (size_t j = i + 1; j < neuralNodes.size(); ++j)
                state.couplingStrength += std::abs (neuralCouplings[i][j]);
        
        return state;
    }

    void setDispersionAmount (float amount)
    {
        dispersionAmount = juce::jlimit (0.0f, 1.0f, amount);
    }

private:
    static constexpr size_t numNodes = 8;
    
    struct NeuralNode
    {
        std::vector<float> delayLine;
        size_t writePos = 0;
        float energy = 0.0f;
        float activation = 0.0f;
        float targetDelayLength = 1024.0f;
        float currentDelayLength = 1024.0f;
    };

    void initializeNeuralCouplings()
    {
        // Initialize with small random weights
        for (size_t i = 0; i < numNodes; ++i)
        {
            for (size_t j = 0; j < numNodes; ++j)
            {
                if (i != j)
                {
                    // Sparse, asymmetric coupling for complex behavior
                    const float prob = juce::Random::getSystemRandom().nextFloat();
                    neuralCouplings[i][j] = (prob > 0.7f) ? 
                        (juce::Random::getSystemRandom().nextFloat() * 0.2f - 0.1f) : 0.0f;
                }
                else
                {
                    neuralCouplings[i][j] = 0.0f;
                }
            }
        }
    }

    void updateNeuralCouplings()
    {
        // Hebbian-like learning: strengthen connections between active nodes
        constexpr float learningRate = 0.0001f;
        
        for (size_t i = 0; i < numNodes; ++i)
        {
            for (size_t j = i + 1; j < numNodes; ++j)
            {
                const float correlation = neuralNodes[i].activation * neuralNodes[j].activation;
                
                // Symmetric update
                neuralCouplings[i][j] += learningRate * correlation;
                neuralCouplings[j][i] += learningRate * correlation;
                
                // Weight decay to prevent runaway
                neuralCouplings[i][j] *= 0.9999f;
                neuralCouplings[j][i] *= 0.9999f;
                
                // Clamp weights
                neuralCouplings[i][j] = juce::jlimit (-0.3f, 0.3f, neuralCouplings[i][j]);
                neuralCouplings[j][i] = juce::jlimit (-0.3f, 0.3f, neuralCouplings[j][i]);
            }
        }
    }

    void updateDispersion (float brightness)
    {
        // Brightness controls dispersion filter characteristics
        const float cutoff = 500.0f + brightness * 8000.0f;
        const float q = 0.5f + brightness * 0.8f;
        
        for (auto& filter : dispersionFilters)
        {
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (
                currentSampleRate, cutoff, q);
            filter.coefficients = coeffs;
        }
    }

    void applyDispersion (juce::AudioBuffer<float>& buffer)
    {
        if (dispersionAmount < 0.01f)
            return;
        
        auto* data = buffer.getWritePointer (0);
        const int numSamples = buffer.getNumSamples();
        
        // Apply cascaded filters for frequency-dependent dispersion
        for (auto& filter : dispersionFilters)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float input = data[i];
                const float filtered = filter.processSample (input);
                data[i] = input + (filtered - input) * dispersionAmount;
            }
        }
    }

    ::Logger& logger;
    
    double currentSampleRate = 44100.0;
    
    std::array<NeuralNode, numNodes> neuralNodes {};
    std::array<std::array<float, numNodes>, numNodes> neuralCouplings {};
    
    std::array<juce::dsp::IIR::Filter<float>, 3> dispersionFilters {};
    
    juce::AudioBuffer<float> tempBuffer;
    
    float excitationEnergy = 0.0f;
    float dispersionAmount = 0.5f;
    
    std::atomic<bool> prepared { false };
};
