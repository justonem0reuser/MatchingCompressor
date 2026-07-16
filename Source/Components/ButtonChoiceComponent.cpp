#include "ButtonChoiceComponent.h"

ButtonChoiceComponent::ButtonChoiceComponent(
    const juce::StringArray& choices,
    Orientation orientation)
    : orientation(orientation)
{
    currentId.addListener(this);
    const bool isHorizontal = (orientation == Orientation::horizontal);

    for (int i = 0; i < choices.size(); ++i)
    {
        auto* button = buttons.add(new juce::TextButton(choices[i]));

        int edges = 0;
        if (i > 0)
            edges |= isHorizontal ? juce::Button::ConnectedOnLeft  : juce::Button::ConnectedOnTop;
        if (i < choices.size() - 1)
            edges |= isHorizontal ? juce::Button::ConnectedOnRight : juce::Button::ConnectedOnBottom;
        button->setConnectedEdges(edges);

        button->onClick = [this, i] { setSelectedItemIndex(i, juce::sendNotificationSync); };
        addAndMakeVisible(button);
    }
}

void ButtonChoiceComponent::setSelectedItemIndex(int newIndex, juce::NotificationType notification)
{
    setSelectedId(newIndex + 1, notification);
}

void ButtonChoiceComponent::setSelectedId(int newItemId, juce::NotificationType notification)
{
    if (lastCurrentId != newItemId)
    {
        lastCurrentId = newItemId;
        currentId = newItemId;

        updateButtonToggleStates();
        repaint();

        sendChange(notification);
    }
}

void ButtonChoiceComponent::valueChanged(juce::Value&)
{
    if (lastCurrentId != (int) currentId.getValue())
        setSelectedId((int) currentId.getValue());
}

void ButtonChoiceComponent::sendChange(juce::NotificationType notification)
{
    if (notification != juce::dontSendNotification)
        triggerAsyncUpdate();

    if (notification == juce::sendNotificationSync)
        handleUpdateNowIfNeeded();
}

void ButtonChoiceComponent::handleAsyncUpdate()
{
    juce::Component::BailOutChecker checker(this);
    listeners.callChecked(checker, [this](Listener& l) { l.buttonChoiceChanged(this); });

    if (checker.shouldBailOut())
        return;

    juce::NullCheckedInvocation::invoke(onChange);
}

void ButtonChoiceComponent::updateButtonToggleStates()
{
    const int selectedIndex = getSelectedItemIndex();
    for (int i = 0; i < buttons.size(); i++)
        buttons[i]->setToggleState(i == selectedIndex, juce::dontSendNotification);
}

void ButtonChoiceComponent::setItemEnabled(int itemId, bool shouldBeEnabled)
{
    const int index = itemId - 1;
    if (index >= 0 && index < buttons.size())
        buttons[index]->setEnabled(shouldBeEnabled);
}

void ButtonChoiceComponent::changeItemText(int itemId, const juce::String& newText)
{
    const int index = itemId - 1;
    if (index >= 0 && index < buttons.size())
        buttons[index]->setButtonText(newText);
}

void ButtonChoiceComponent::resized()
{
    if (buttons.isEmpty())
        return;

    auto bounds = getLocalBounds();
    const bool isHorizontal = (orientation == Orientation::horizontal);
    const int total = isHorizontal ? bounds.getWidth() : bounds.getHeight();
    const int n = buttons.size();

    for (int i = 0; i < n; ++i)
    {
        const int p0 = i * total / n;
        const int p1 = (i + 1) * total / n;
        if (isHorizontal)
            buttons[i]->setBounds(bounds.getX() + p0, bounds.getY(), p1 - p0, bounds.getHeight());
        else
            buttons[i]->setBounds(bounds.getX(), bounds.getY() + p0, bounds.getWidth(), p1 - p0);
    }
}
