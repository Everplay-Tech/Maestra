#pragma once

#include <JuceHeader.h>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include "../Engine/OrchestraSynthEngine.h"
#include "../Systems/PresetManager.h"
#include "../Systems/PerformanceMonitor.h"
#include "../Systems/Logger.h"
#include "SectionStripComponent.h"
#include "PresetBar.h"

class MixerComponent : public juce::Component,
                       private juce::KeyListener
{
public:
    MixerComponent (OrchestraSynthEngine& engineIn,
                    PresetManager& presetManagerIn,
                    PerformanceMonitor& perfMonIn,
                    ::Logger& loggerIn);
    ~MixerComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void parentHierarchyChanged() override;
    void focusLost (FocusChangeType cause) override;
    bool keyPressed (const juce::KeyPress& key) override;
    bool keyStateChanged (bool isKeyDown) override;

    // ...

private:
    class KeyboardFocusSentinel;
    class TypingOverlayComponent;
    class VirtualKeyboardDock;

    struct ActiveKeyState
    {
        bool isDown = false;
        int midiNote = 0;
        std::vector<int> channels;
    };

    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged (bool isKeyDown, juce::Component* originatingComponent) override;

    bool shouldAcceptKeyboardInput() const;
    int getNormalisedKeyCode (const juce::KeyPress& key) const;
    bool handleNoteKey (int keyCode);
    bool handleOctaveKey (int keyCode);
    bool handleMultitimbralKey (int keyCode);
    void refreshReleasedKeys();
    void sendNoteOn (int midiNote, ActiveKeyState& state);
    void sendNoteOff (const ActiveKeyState& state);
    void allNotesOffFromKeyboard();
    void triggerVirtualKeyboardNote (int midiNote, float velocity, bool isNoteOn);
    std::vector<int> buildChannelListForNewNote() const;
    void ensureKeyboardDock();
    void updateTypingOverlay();
    void updateSectionTypingHighlights();
    void adjustTypingActivityForChannels (const std::vector<int>& channels, int delta);
    void setKeyboardDrawerVisible (bool shouldShow);
    void configureStatusLabel (juce::Label& label, const juce::String& text);

    OrchestraSynthEngine& engine;
    [[maybe_unused]] PresetManager& presetManager;
    [[maybe_unused]] PerformanceMonitor& perfMon;
    ::Logger& logger;

    PresetBar presetBar;
    SectionStripComponent stringsStrip;
    SectionStripComponent brassStrip;
    SectionStripComponent woodwindsStrip;
    SectionStripComponent percussionStrip;
    SectionStripComponent choirStrip;

    juce::Label shortcutHint;
    juce::Label octaveStatusLabel;
    juce::Label layerStatusLabel;
    juce::ToggleButton keyboardToggle { "Show Keyboard" };
    std::unique_ptr<KeyboardFocusSentinel> keyboardSentinel;
    std::unique_ptr<TypingOverlayComponent> typingOverlay;
    std::unique_ptr<VirtualKeyboardDock> keyboardDock;

    std::unordered_map<int, ActiveKeyState> activeKeyStates;
    std::unordered_map<int, std::vector<int>> virtualKeyboardActiveNotes;
    std::vector<int> playableKeyCodes;
    std::unordered_set<int> activeTypingKeyCodes;
    std::array<int, OrchestraSynthEngine::numSections> sectionTypingHolds {};
    int keyboardBaseNote = 60;
    int keyboardOctaveOffset = 0;
    int multitimbralCount = 1;
    bool keyboardDockVisible = false;
    int keyboardDrawerHeight = 210;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerComponent)
};
