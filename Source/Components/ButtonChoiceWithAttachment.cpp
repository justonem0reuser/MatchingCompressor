#include "ButtonChoiceWithAttachment.h"

ButtonChoiceWithAttachment::ButtonChoiceWithAttachment(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& id,
    const juce::StringArray& choices,
    Orientation orientation)
    : ButtonChoiceComponent(choices, orientation)
{
    auto name = apvts.getParameter(id)->getName(1000);
    setName(name);
    setTitle(name); 

    attachment = std::make_unique<ButtonChoiceAttachment>(apvts, id, *this);
}
