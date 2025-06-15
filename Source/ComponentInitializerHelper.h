#pragma once
#include <JuceHeader.h>

static class ComponentInitializerHelper
{
public:
    static void initLabel(
        juce::Component* component, 
        juce::Label& label,
        juce::Component* componentToAttach = nullptr,
        bool onLeft = true);
    static void initLabelAndTextBox(
        juce::Component* component, 
        juce::Label& label, 
        const juce::String& labelText, 
        juce::Label& textBox, 
        const juce::String& textBoxText,
        std::function<void()> action);
    static void initTextButton(
        juce::Component* component, 
        juce::TextButton& button, 
        const juce::String& text, 
        std::function<void()> action);
    static void initToggleButton(
        juce::Component* component, 
        juce::ToggleButton& button, 
        const juce::String& text,
        bool initState, 
        bool sendChange, 
        std::function<void()> action);
};