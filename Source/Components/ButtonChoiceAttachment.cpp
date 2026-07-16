#include "ButtonChoiceAttachment.h"

ButtonChoiceAttachment::ButtonChoiceAttachment(
    juce::AudioProcessorValueTreeState& stateToUse,
    const juce::String& parameterID,
    ButtonChoiceComponent& buttonChoice)
{
    if (auto* parameter = stateToUse.getParameter(parameterID))
        attachment = std::make_unique<ButtonChoiceParameterAttachment>(*parameter, buttonChoice);
    else
        jassertfalse; // this parameter does not exist in the APVTS
}
