#include "ComponentInitializerHelper.h"

void ComponentInitializerHelper::initLabel(
    juce::Component* component,
    juce::Label& label,
    juce::Component* componentToAttach,
    bool onLeft)
{
    component->addAndMakeVisible(&label);
    label.setFont(juce::Font(16.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::left);
    if (componentToAttach != nullptr)
    {
        label.setText(componentToAttach->getTitle(), juce::NotificationType::dontSendNotification);
        label.attachToComponent(componentToAttach, onLeft);
    }
}

void ComponentInitializerHelper::initTextButton(
    juce::Component* component, 
    juce::TextButton& button, 
    const juce::String& text, 
    std::function<void()> action)
{
    component->addAndMakeVisible(&button);
    button.setButtonText(text);
    button.onClick = action;
}
