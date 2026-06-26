#pragma once
#include <JuceHeader.h>

class ComponentInitializerHelper
{
public:
    static void initLabel(
        juce::Component* component, 
        juce::Label& label,
        juce::Component* componentToAttach = nullptr,
        bool onLeft = true);
    static void initTextButton(
        juce::Component* component, 
        juce::TextButton& button, 
        const juce::String& text, 
        std::function<void()> action);
};