#pragma once
#include <JuceHeader.h>

// Buttons analogue of juce::ComboBox
class ButtonChoiceComponent : public juce::Component,
                              private juce::Value::Listener,
                              private juce::AsyncUpdater
{
public:
    enum class Orientation { horizontal, vertical };

    explicit ButtonChoiceComponent(
        const juce::StringArray& choices,
        Orientation orientation = Orientation::horizontal);

    int getNumItems() const noexcept           { return buttons.size(); }

    // 1-based item id (0 is nothing selected)
    int getSelectedId() const noexcept         { return (int)currentId.getValue(); }
    // 0-based index
    int getSelectedItemIndex() const noexcept  { return getSelectedId() - 1; }

    void setSelectedItemIndex(int newIndex, juce::NotificationType notification = juce::sendNotificationAsync);
    void setSelectedId(int newItemId, juce::NotificationType notification = juce::sendNotificationAsync);

    juce::Value& getSelectedIdAsValue() noexcept { return currentId; }

    void setItemEnabled(int itemId, bool shouldBeEnabled);
    void changeItemText(int itemId, const juce::String& newText);

    std::function<void()> onChange;

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void buttonChoiceChanged(ButtonChoiceComponent* componentThatHasChanged) = 0;
    };

    void addListener(Listener* listener)      { listeners.add(listener); }
    void removeListener(Listener* listener)   { listeners.remove(listener); }

    void resized() override;

private:
    void valueChanged(juce::Value&) override;
    void handleAsyncUpdate() override;
    void sendChange(juce::NotificationType notification);
    void updateButtonToggleStates();

    juce::OwnedArray<juce::TextButton> buttons;
    const Orientation orientation;

    juce::Value currentId;
    int lastCurrentId = 0;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonChoiceComponent)
};
