#pragma once
#include <JuceHeader.h>
#include "ButtonChoiceComponent.h"

// Button-based analogue of juce::ChoicePropertyComponent
class ButtonChoicePropertyComponent : public juce::PropertyComponent
{
public:
    ButtonChoicePropertyComponent(
        const juce::Value& valueToControl,
        const juce::String& propertyName,
        const juce::StringArray& choices,
        const juce::Array<juce::var>& correspondingValues);

    void refresh() override;

private:
    ButtonChoiceComponent buttonChoice;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonChoicePropertyComponent)
};
