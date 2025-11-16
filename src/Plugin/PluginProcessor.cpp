#include "PluginProcessor.h"
#include "PluginEditor.h"

OrchestraSynthAudioProcessor::OrchestraSynthAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

const juce::String OrchestraSynthAudioProcessor::getName() const
{
    return "OrchestraSynth";
}

void OrchestraSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
}

void OrchestraSynthAudioProcessor::releaseResources()
{
    engine.reset();
}

bool OrchestraSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void OrchestraSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    if (getTotalNumInputChannels() < buffer.getNumChannels())
        buffer.clear (getTotalNumInputChannels(),
                      0,
                      buffer.getNumSamples());

    engine.processBlock (buffer, midi);
}

juce::AudioProcessorEditor* OrchestraSynthAudioProcessor::createEditor()
{
    return new OrchestraSynthAudioProcessorEditor (*this);
}

void OrchestraSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree root ("orchestraSynthState");
    juce::ValueTree sections ("sections");
    root.addChild (sections, -1, nullptr);
    engine.serialiseToValueTree (sections);

    juce::MemoryOutputStream out (destData, false);
    root.writeToStream (out);
}

void OrchestraSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream in (data, (size_t) sizeInBytes, false);
    auto root = juce::ValueTree::readFromStream (in);
    if (! root.isValid())
        return;

    auto sections = root.getChildWithName (juce::Identifier ("sections"));
    if (sections.isValid())
        engine.deserialiseFromValueTree (sections);
}
