#pragma once

#include <JuceHeader.h>
#include "Logger.h"

class OrchestraSynthEngine; // forward declaration

class PresetManager
{
public:
    PresetManager() = default;

    PresetManager (const PresetManager&) = delete;
    PresetManager& operator= (const PresetManager&) = delete;
    PresetManager (PresetManager&&) = delete;
    PresetManager& operator= (PresetManager&&) = delete;

    void savePreset (const juce::String& name, const OrchestraSynthEngine& engine)
    {
        juce::ValueTree presetTree (juce::Identifier ("orchestraPreset"));
        presetTree.setProperty (juce::Identifier ("name"), name, nullptr);

        juce::ValueTree sectionsTree (juce::Identifier ("sections"));
        presetTree.addChild (sectionsTree, -1, nullptr);

        engine.serialiseToValueTree (sectionsTree);
        presets.setProperty (juce::Identifier (name), presetTree, nullptr);
    }

    void loadPreset (const juce::String& name, OrchestraSynthEngine& engine)
    {
        auto var = presets.getProperty (juce::Identifier (name));
        if (! var.isObject())
            return;

        juce::ValueTree presetTree = var;
        auto sections = presetTree.getChildWithName (juce::Identifier ("sections"));
        if (sections.isValid())
            engine.deserialiseFromValueTree (sections);
    }

    juce::StringArray getPresetNames() const
    {
        juce::StringArray names;

        auto props = presets.getDynamicObject();
        if (props != nullptr)
        {
            const auto& p = props->getProperties();
            for (auto& k : p.getAllKeys())
                names.add (k.toString());
        }

        return names;
    }

private:
    juce::NamedValueSet presets;
};
