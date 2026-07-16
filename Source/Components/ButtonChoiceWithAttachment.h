#pragma once
#include <JuceHeader.h>
#include "ButtonChoiceComponent.h"
#include "ButtonChoiceAttachment.h"

class ButtonChoiceWithAttachment : public ButtonChoiceComponent
{
public:
    ButtonChoiceWithAttachment(
        juce::AudioProcessorValueTreeState& apvts,
        const juce::String& id,
        const juce::StringArray& choices,
        Orientation orientation = Orientation::horizontal);

private:
    std::unique_ptr<ButtonChoiceAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonChoiceWithAttachment)
};
