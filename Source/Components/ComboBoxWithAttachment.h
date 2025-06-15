#pragma once
#include <JuceHeader.h>

class ComboBoxWithAttachment : public juce::ComboBox
{
public:
    ComboBoxWithAttachment(
        juce::AudioProcessorValueTreeState& apvts, 
        const juce::String& id, 
        const juce::StringArray choices);

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComboBoxWithAttachment)
};
