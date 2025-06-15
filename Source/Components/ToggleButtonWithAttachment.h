#pragma once

#include <JuceHeader.h>

class ToggleButtonWithAttachment : public juce::ToggleButton
{
public:
    ToggleButtonWithAttachment(juce::AudioProcessorValueTreeState& apvts, const juce::String& id);

private:
    juce::AudioProcessorValueTreeState::ButtonAttachment attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleButtonWithAttachment)
};
