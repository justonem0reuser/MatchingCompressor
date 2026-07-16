#pragma once
#include <JuceHeader.h>
#include "ButtonChoiceComponent.h"
#include "ButtonChoiceParameterAttachment.h"

// Analogue of juce::AudioProcessorValueTreeState::ComboBoxAttachment
class ButtonChoiceAttachment
{
public:
    ButtonChoiceAttachment(juce::AudioProcessorValueTreeState& stateToUse,
                           const juce::String& parameterID,
                           ButtonChoiceComponent& buttonChoice);

private:
    std::unique_ptr<ButtonChoiceParameterAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonChoiceAttachment)
};
