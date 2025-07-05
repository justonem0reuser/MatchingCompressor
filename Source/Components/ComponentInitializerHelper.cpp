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

void ComponentInitializerHelper::initLabelAndTextBox(
    juce::Component* component, 
    juce::Label& label, 
    const juce::String& labelText, 
    juce::Label& textBox, 
    const juce::String& textBoxText,
    std::function<void()> action)
{
    component->addAndMakeVisible(&label);
    label.setFont(juce::Font(16.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    label.setJustificationType(juce::Justification::left);
    label.setText(labelText, juce::NotificationType::dontSendNotification);
    label.attachToComponent(&textBox, true);
    component->addAndMakeVisible(&textBox);
    textBox.setFont(juce::Font(16.0f, juce::Font::bold));
    textBox.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    textBox.setText(textBoxText, juce::NotificationType::dontSendNotification);
    textBox.setEditable(true);
    textBox.onTextChange = action;
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

void ComponentInitializerHelper::initToggleButton(
    juce::Component* component, 
    juce::ToggleButton& button, 
    const juce::String& text,
    bool initState, 
    bool sendChange, 
    std::function<void()> action)
{
    component->addAndMakeVisible(&button);
    button.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgreen);
    button.setButtonText(text);
    button.setToggleState(initState, sendChange);
    button.onClick = action;
}
