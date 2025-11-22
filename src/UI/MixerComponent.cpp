#include "MixerComponent.h"

namespace
{
struct KeyMapping
{
    juce_wchar keyChar;
    int semitoneOffset;
};

constexpr KeyMapping whiteKeyMappings[] =
{
    { 'a', 0 },  { 's', 2 },  { 'd', 4 },  { 'f', 5 },  { 'g', 7 },
    { 'h', 9 },  { 'j', 11 }, { 'k', 12 }, { 'l', 14 }, { ';', 16 }
};

constexpr KeyMapping blackKeyMappings[] =
{
    { 'w', 1 },  { 'e', 3 },  { 'r', 6 },  { 't', 8 },
    { 'u', 10 }, { 'i', 13 }, { 'o', 15 }
};

const KeyMapping* findMappingForKey (int keyCode)
{
    const auto lowered = juce::CharacterFunctions::toLowerCase ((juce_wchar) keyCode);

    for (const auto& mapping : whiteKeyMappings)
        if (mapping.keyChar == lowered)
            return &mapping;

    for (const auto& mapping : blackKeyMappings)
        if (mapping.keyChar == lowered)
            return &mapping;

    return nullptr;
}

bool isKeyCurrentlyDownForCode (int keyCode)
{
    if (juce::KeyPress::isKeyCurrentlyDown (keyCode))
        return true;

    const auto upper = (int) juce::CharacterFunctions::toUpperCase ((juce_wchar) keyCode);
    if (upper != keyCode && juce::KeyPress::isKeyCurrentlyDown (upper))
        return true;

    return false;
}

class VirtualKeyboardContent : public juce::Component,
                               private juce::MidiKeyboardStateListener
{
public:
    using NoteCallback = std::function<void (int midiNote, float velocity, bool isNoteOn)>;

    explicit VirtualKeyboardContent (NoteCallback cb)
        : callback (std::move (cb)),
          keyboardComponent (keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        keyboardComponent.setAvailableRange (36, 96);
        keyboardComponent.setKeyPressBaseOctave (4);
        keyboardState.addListener (this);
        addAndMakeVisible (keyboardComponent);

        infoLabel.setText ("Click keys or play a MIDI device to audition OrchestraSynth",
                           juce::dontSendNotification);
        infoLabel.setJustificationType (juce::Justification::centred);
        infoLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible (infoLabel);
    }

    ~VirtualKeyboardContent() override
    {
        keyboardState.removeListener (this);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        infoLabel.setBounds (area.removeFromTop (24));
        keyboardComponent.setBounds (area);
    }

private:
    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (callback != nullptr)
            callback (midiNoteNumber, velocity, true);
        juce::ignoreUnused (midiChannel);
    }

    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float) override
    {
        if (callback != nullptr)
            callback (midiNoteNumber, 0.0f, false);
        juce::ignoreUnused (midiChannel);
    }

    NoteCallback callback;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;
    juce::Label infoLabel;
};

} // namespace

class MixerComponent::VirtualKeyboardDock : public juce::Component
{
public:
    using NoteCallback = VirtualKeyboardContent::NoteCallback;

    explicit VirtualKeyboardDock (NoteCallback cb)
        : content (std::move (cb))
    {
        addAndMakeVisible (content);
    }

    void resized() override
    {
        content.setBounds (getLocalBounds().reduced (6));
    }

private:
    VirtualKeyboardContent content;
};

class MixerComponent::TypingOverlayComponent : public juce::Component
{
public:
    TypingOverlayComponent()
    {
        setInterceptsMouseClicks (false, false);

        auto makeDisplay = [] (juce_wchar c)
        {
            TypingOverlayComponent::KeyDisplay d;
            d.label = juce::String::charToString (juce::CharacterFunctions::toUpperCase (c));
            d.keyCode = (int) c;
            return d;
        };

        for (const auto& mapping : whiteKeyMappings)
            whiteKeys.push_back (makeDisplay (mapping.keyChar));

        for (const auto& mapping : blackKeyMappings)
            blackKeys.push_back (makeDisplay (mapping.keyChar));
    }

    void setActiveKeyCodes (const std::unordered_set<int>& codes)
    {
        activeKeyCodes = codes;
        repaint();
    }

    void setOctaveShift (int shift)
    {
        if (octaveShift != shift)
        {
            octaveShift = shift;
            repaint();
        }
    }

    void setLayerCount (int count)
    {
        count = juce::jlimit (1, 5, count);
        if (layerCount != count)
        {
            layerCount = count;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().reduced (4);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (area.toFloat(), 6.0f);

        auto header = area.removeFromTop (20);
        auto font = g.getCurrentFont().withHeight (14.0f);
        g.setFont (font);
        g.setColour (juce::Colours::lightgrey);
        g.drawText ("Typing Keys", header.removeFromLeft (130), juce::Justification::centredLeft);
        g.drawText ("Octave shift: " + juce::String (octaveShift),
                    header.removeFromLeft (160),
                    juce::Justification::centredLeft);
        g.drawText ("Layers: " + juce::String (layerCount),
                    header,
                    juce::Justification::centredRight);

        auto keyRowHeight = area.getHeight() / 2;
        drawKeyRow (g, area.removeFromTop (keyRowHeight), whiteKeys, false);
        drawKeyRow (g, area, blackKeys, true);
    }

private:
    struct KeyDisplay
    {
        juce::String label;
        int keyCode = 0;
    };

    void drawKeyRow (juce::Graphics& g,
                     juce::Rectangle<int> row,
                     const std::vector<KeyDisplay>& keys,
                     bool isBlackRow)
    {
        if (keys.empty())
            return;

        auto keyWidth = row.getWidth() / (int) keys.size();
        for (const auto& key : keys)
        {
            auto cell = row.removeFromLeft (keyWidth);
            auto isActive = activeKeyCodes.find (key.keyCode) != activeKeyCodes.end();
            auto baseColour = isBlackRow ? juce::Colours::darkslategrey : juce::Colours::dimgrey;
            auto fillColour = isActive ? juce::Colours::orange : baseColour;

            g.setColour (fillColour);
            g.fillRoundedRectangle (cell.reduced (3).toFloat(), 4.0f);

            g.setColour (juce::Colours::white);
            g.drawText (key.label, cell, juce::Justification::centred);
        }
    }

    std::vector<KeyDisplay> whiteKeys;
    std::vector<KeyDisplay> blackKeys;
    std::unordered_set<int> activeKeyCodes;
    int octaveShift = 0;
    int layerCount = 1;
};

class MixerComponent::KeyboardFocusSentinel : public juce::Component
{
public:
    explicit KeyboardFocusSentinel (MixerComponent& ownerIn)
        : owner (ownerIn)
    {
        setInterceptsMouseClicks (false, false);
        setWantsKeyboardFocus (true);
        setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    }

    void parentHierarchyChanged() override
    {
        if (isShowing())
            grabKeyboardFocus();
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        grabKeyboardFocus();
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        return owner.keyPressed (key);
    }

    bool keyStateChanged (bool isKeyDown) override
    {
        return owner.keyStateChanged (isKeyDown);
    }

private:
    MixerComponent& owner;
};

MixerComponent::MixerComponent (OrchestraSynthEngine& engineIn,
                                PresetManager& presetManagerIn,
                                PerformanceMonitor& perfMonIn,
                                ::Logger& loggerIn)
    : engine (engineIn),
      presetManager (presetManagerIn),
      perfMon (perfMonIn),
      logger (loggerIn),
      presetBar (engineIn, presetManagerIn, perfMonIn, loggerIn),
      stringsStrip (engineIn, OrchestraSynthEngine::Strings,    "Strings"),
      brassStrip (engineIn,   OrchestraSynthEngine::Brass,      "Brass"),
      woodwindsStrip (engineIn, OrchestraSynthEngine::Woodwinds,"Woodwinds"),
      percussionStrip (engineIn, OrchestraSynthEngine::Percussion, "Percussion"),
      choirStrip (engineIn,   OrchestraSynthEngine::Choir,      "Choir")
{
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    setWantsKeyboardFocus (true);
    addKeyListener (this);

    for (const auto& mapping : whiteKeyMappings)
        playableKeyCodes.push_back ((int) mapping.keyChar);
    for (const auto& mapping : blackKeyMappings)
        playableKeyCodes.push_back ((int) mapping.keyChar);

    addAndMakeVisible (presetBar);
    addAndMakeVisible (stringsStrip);
    addAndMakeVisible (brassStrip);
    addAndMakeVisible (woodwindsStrip);
    addAndMakeVisible (percussionStrip);
    addAndMakeVisible (choirStrip);

    configureStatusLabel (shortcutHint,
                          "Typing Keys: ASDFGHJKL; (white)  WERTUIO (black)  |  X/C octaves  |  V/B layers");
    shortcutHint.setJustificationType (juce::Justification::centredLeft);
    configureStatusLabel (octaveStatusLabel, "Octave shift: 0");
    octaveStatusLabel.setJustificationType (juce::Justification::centred);
    configureStatusLabel (layerStatusLabel, "Layers: 1");
    layerStatusLabel.setJustificationType (juce::Justification::centredRight);

    keyboardToggle.setClickingTogglesState (true);
    keyboardToggle.setWantsKeyboardFocus (false);
    keyboardToggle.onClick = [this]
    {
        ensureKeyboardDock();
        setKeyboardDrawerVisible (keyboardToggle.getToggleState());
    };
    addAndMakeVisible (keyboardToggle);

    typingOverlay.reset (new TypingOverlayComponent());
    addAndMakeVisible (*typingOverlay);

    keyboardSentinel.reset (new KeyboardFocusSentinel (*this));
    addAndMakeVisible (*keyboardSentinel);

    updateTypingOverlay();
}

MixerComponent::~MixerComponent()
{
    removeKeyListener (this);
    allNotesOffFromKeyboard();
}

void MixerComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void MixerComponent::resized()
{
    auto area = getLocalBounds();

    auto keyboardAreaHeight = keyboardDockVisible ? keyboardDrawerHeight : 0;
    if (keyboardDock != nullptr)
    {
        if (keyboardAreaHeight > 0)
        {
            auto dockArea = area.removeFromBottom (keyboardAreaHeight);
            keyboardDock->setBounds (dockArea.reduced (6));
            keyboardDock->setVisible (true);
        }
        else
        {
            keyboardDock->setVisible (false);
        }
    }

    auto overlayBlock = area.removeFromBottom (110);
    auto statusRow = overlayBlock.removeFromTop (24);
    auto statusWidth = statusRow.getWidth() / 3;
    shortcutHint.setBounds (statusRow.removeFromLeft (statusWidth));
    octaveStatusLabel.setBounds (statusRow.removeFromLeft (statusWidth));
    layerStatusLabel.setBounds (statusRow);

    auto overlayBounds = overlayBlock.reduced (4);
    if (typingOverlay != nullptr)
        typingOverlay->setBounds (overlayBounds);

    if (keyboardSentinel != nullptr)
    {
        auto sentinelArea = overlayBounds.removeFromBottom (10);
        keyboardSentinel->setBounds (sentinelArea.expanded (0, 4));
    }

    auto top = area.removeFromTop (40);
    auto toggleWidth = 180;
    presetBar.setBounds (top.removeFromLeft (top.getWidth() - toggleWidth).reduced (0, 2));
    keyboardToggle.setBounds (top.removeFromLeft (toggleWidth).reduced (4, 0));

    auto stripArea = area.reduced (8, 8);
    const int stripWidth = stripArea.getWidth() / 5;

    stringsStrip.setBounds    (stripArea.removeFromLeft (stripWidth).reduced (4));
    brassStrip.setBounds      (stripArea.removeFromLeft (stripWidth).reduced (4));
    woodwindsStrip.setBounds  (stripArea.removeFromLeft (stripWidth).reduced (4));
    percussionStrip.setBounds (stripArea.removeFromLeft (stripWidth).reduced (4));
    choirStrip.setBounds      (stripArea.reduced (4));

    if (keyboardSentinel != nullptr && isShowing())
        keyboardSentinel->grabKeyboardFocus();
}

void MixerComponent::parentHierarchyChanged()
{
    if (isShowing())
    {
        grabKeyboardFocus();
        if (keyboardSentinel != nullptr)
            keyboardSentinel->grabKeyboardFocus();
    }
}

void MixerComponent::focusLost (FocusChangeType)
{
    allNotesOffFromKeyboard();
}

bool MixerComponent::keyPressed (const juce::KeyPress& key)
{
    return keyPressed (key, nullptr);
}

bool MixerComponent::keyStateChanged (bool isKeyDown)
{
    return keyStateChanged (isKeyDown, nullptr);
}

bool MixerComponent::keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent)
{
    if (! shouldAcceptKeyboardInput())
        return false;

    if (originatingComponent != nullptr && dynamic_cast<juce::TextEditor*> (originatingComponent) != nullptr)
        return false;

    const auto normalisedCode = getNormalisedKeyCode (key);

    if (handleOctaveKey (normalisedCode))
        return true;

    if (handleMultitimbralKey (normalisedCode))
        return true;

    if (handleNoteKey (normalisedCode))
        return true;

    return false;
}

bool MixerComponent::keyStateChanged (bool, juce::Component*)
{
    refreshReleasedKeys();
    return false;
}

bool MixerComponent::shouldAcceptKeyboardInput() const
{
    if (! isShowing())
        return false;

    if (isCurrentlyBlockedByAnotherModalComponent())
        return false;

    return true;
}

int MixerComponent::getNormalisedKeyCode (const juce::KeyPress& key) const
{
    auto textChar = key.getTextCharacter();
    if (textChar != 0)
        return (int) juce::CharacterFunctions::toLowerCase (textChar);

    auto code = key.getKeyCode();
    if (code >= 'A' && code <= 'Z')
        return code + 32;

    return code;
}

bool MixerComponent::handleNoteKey (int keyCode)
{
    if (const auto* mapping = findMappingForKey (keyCode))
    {
        auto& state = activeKeyStates[keyCode];
        if (state.isDown)
            return true;

        const int midiNote = juce::jlimit (0, 127, keyboardBaseNote + keyboardOctaveOffset + mapping->semitoneOffset);
        state.isDown = true;
        state.midiNote = midiNote;
        sendNoteOn (midiNote, state);
        activeTypingKeyCodes.insert (keyCode);
        updateTypingOverlay();
        return true;
    }

    return false;
}

bool MixerComponent::handleOctaveKey (int keyCode)
{
    static constexpr int octaveStep = 12;
    static constexpr int minOffset = -36;
    static constexpr int maxOffset = 36;

    if (keyCode == 'x')
    {
        const auto previous = keyboardOctaveOffset;
        keyboardOctaveOffset = juce::jlimit (minOffset, maxOffset, keyboardOctaveOffset - octaveStep);
        if (keyboardOctaveOffset != previous)
        {
            logger.log (::Logger::LogLevel::Info,
                        "Keyboard octave decreased to shift " + juce::String (keyboardOctaveOffset / octaveStep));
            allNotesOffFromKeyboard();
        }
        updateTypingOverlay();
        return true;
    }

    if (keyCode == 'c')
    {
        const auto previous = keyboardOctaveOffset;
        keyboardOctaveOffset = juce::jlimit (minOffset, maxOffset, keyboardOctaveOffset + octaveStep);
        if (keyboardOctaveOffset != previous)
        {
            logger.log (::Logger::LogLevel::Info,
                        "Keyboard octave increased to shift " + juce::String (keyboardOctaveOffset / octaveStep));
            allNotesOffFromKeyboard();
        }
        updateTypingOverlay();
        return true;
    }

    return false;
}

bool MixerComponent::handleMultitimbralKey (int keyCode)
{
    if (keyCode == 'b')
    {
        const auto previous = multitimbralCount;
        multitimbralCount = juce::jlimit (1, OrchestraSynthEngine::numSections, multitimbralCount + 1);
        if (multitimbralCount != previous)
            logger.log (::Logger::LogLevel::Info,
                        "Multitimbral layer increased to " + juce::String (multitimbralCount));
        updateTypingOverlay();
        return true;
    }

    if (keyCode == 'v')
    {
        const auto previous = multitimbralCount;
        multitimbralCount = juce::jlimit (1, OrchestraSynthEngine::numSections, multitimbralCount - 1);
        if (multitimbralCount != previous)
            logger.log (::Logger::LogLevel::Info,
                        "Multitimbral layer decreased to " + juce::String (multitimbralCount));
        updateTypingOverlay();
        return true;
    }

    return false;
}

void MixerComponent::refreshReleasedKeys()
{
    bool anyKeyReleased = false;

    for (auto keyCode : playableKeyCodes)
    {
        if (auto it = activeKeyStates.find (keyCode); it != activeKeyStates.end())
        {
            if (! isKeyCurrentlyDownForCode (keyCode))
            {
                sendNoteOff (it->second);
                activeKeyStates.erase (it);
                activeTypingKeyCodes.erase (keyCode);
                anyKeyReleased = true;
            }
        }
    }

    if (anyKeyReleased)
        updateTypingOverlay();
}

void MixerComponent::sendNoteOn (int midiNote, ActiveKeyState& state)
{
    state.channels = buildChannelListForNewNote();
    adjustTypingActivityForChannels (state.channels, +1);

    static constexpr juce::uint8 defaultVelocity = 110;
    for (auto channel : state.channels)
        engine.postVirtualMidiMessage (juce::MidiMessage::noteOn (channel, midiNote, defaultVelocity));
}

void MixerComponent::sendNoteOff (const ActiveKeyState& state)
{
    adjustTypingActivityForChannels (state.channels, -1);
    for (auto channel : state.channels)
        engine.postVirtualMidiMessage (juce::MidiMessage::noteOff (channel, state.midiNote));
}

void MixerComponent::allNotesOffFromKeyboard()
{
    for (auto& entry : activeKeyStates)
    {
        if (entry.second.isDown)
            sendNoteOff (entry.second);
    }

    activeKeyStates.clear();

    for (auto& entry : virtualKeyboardActiveNotes)
    {
        for (auto channel : entry.second)
            engine.postVirtualMidiMessage (juce::MidiMessage::noteOff (channel, entry.first));
    }
    virtualKeyboardActiveNotes.clear();

    activeTypingKeyCodes.clear();
    sectionTypingHolds.fill (0);
    updateTypingOverlay();
    updateSectionTypingHighlights();
}

std::vector<int> MixerComponent::buildChannelListForNewNote() const
{
    const auto sectionsToTrigger = juce::jlimit (1, OrchestraSynthEngine::numSections, multitimbralCount);
    std::vector<int> channels;
    channels.reserve ((size_t) sectionsToTrigger);

    for (int channel = 1; channel <= sectionsToTrigger; ++channel)
        channels.push_back (channel);

    return channels;
}

void MixerComponent::triggerVirtualKeyboardNote (int midiNote, float velocity, bool isNoteOn)
{
    velocity = juce::jlimit (0.0f, 1.0f, velocity);

    if (isNoteOn)
    {
        auto& channels = virtualKeyboardActiveNotes[midiNote];
        channels = buildChannelListForNewNote();
        adjustTypingActivityForChannels (channels, +1);

        const juce::uint8 midiVelocity = (juce::uint8) juce::jlimit (0, 127, (int) std::round (velocity * 127.0f));
        for (auto channel : channels)
            engine.postVirtualMidiMessage (juce::MidiMessage::noteOn (channel, midiNote, midiVelocity));
    }
    else
    {
        auto it = virtualKeyboardActiveNotes.find (midiNote);
        if (it == virtualKeyboardActiveNotes.end())
            return;

        for (auto channel : it->second)
            engine.postVirtualMidiMessage (juce::MidiMessage::noteOff (channel, midiNote));

        adjustTypingActivityForChannels (it->second, -1);
        virtualKeyboardActiveNotes.erase (it);
    }
}

void MixerComponent::ensureKeyboardDock()
{
    if (keyboardDock != nullptr)
        return;

    auto noteCallback = [this] (int midiNote, float velocity, bool isNoteOn)
    {
        triggerVirtualKeyboardNote (midiNote, velocity <= 0.0f ? 0.8f : velocity, isNoteOn);
    };

    keyboardDock = std::make_unique<VirtualKeyboardDock> (noteCallback);
    keyboardDock->setVisible (false);
    addAndMakeVisible (*keyboardDock);
    keyboardDock->toBack();

    // Ensure overlays remain in front
    shortcutHint.toFront (false);
    octaveStatusLabel.toFront (false);
    layerStatusLabel.toFront (false);
    if (typingOverlay != nullptr)
        typingOverlay->toFront (false);
    if (keyboardSentinel != nullptr)
        keyboardSentinel->toFront (false);
}

void MixerComponent::setKeyboardDrawerVisible (bool shouldShow)
{
    keyboardDockVisible = shouldShow;
    if (keyboardDock != nullptr)
        keyboardDock->setVisible (keyboardDockVisible);

    resized();
}

void MixerComponent::updateTypingOverlay()
{
    if (typingOverlay != nullptr)
    {
        typingOverlay->setActiveKeyCodes (activeTypingKeyCodes);
        typingOverlay->setOctaveShift (keyboardOctaveOffset / 12);
        typingOverlay->setLayerCount (multitimbralCount);
    }

    octaveStatusLabel.setText ("Octave shift: " + juce::String (keyboardOctaveOffset / 12),
                               juce::dontSendNotification);
    layerStatusLabel.setText ("Layers: " + juce::String (multitimbralCount),
                              juce::dontSendNotification);
}

void MixerComponent::updateSectionTypingHighlights()
{
    stringsStrip.setTypingHighlight (sectionTypingHolds[OrchestraSynthEngine::Strings] > 0);
    brassStrip.setTypingHighlight (sectionTypingHolds[OrchestraSynthEngine::Brass] > 0);
    woodwindsStrip.setTypingHighlight (sectionTypingHolds[OrchestraSynthEngine::Woodwinds] > 0);
    percussionStrip.setTypingHighlight (sectionTypingHolds[OrchestraSynthEngine::Percussion] > 0);
    choirStrip.setTypingHighlight (sectionTypingHolds[OrchestraSynthEngine::Choir] > 0);
}

void MixerComponent::adjustTypingActivityForChannels (const std::vector<int>& channels, int delta)
{
    bool changed = false;

    for (auto channel : channels)
    {
        const int sectionIndex = juce::jlimit (0, OrchestraSynthEngine::numSections - 1, channel - 1);
        auto& counter = sectionTypingHolds[(size_t) sectionIndex];
        const int previous = counter;
        counter = juce::jmax (0, counter + delta);
        if (counter != previous)
            changed = true;
    }

    if (changed)
        updateSectionTypingHighlights();
}

void MixerComponent::configureStatusLabel (juce::Label& label, const juce::String& text)
{
    label.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    label.setFont (juce::Font (13.0f));
    label.setText (text, juce::dontSendNotification);
    addAndMakeVisible (label);
}
