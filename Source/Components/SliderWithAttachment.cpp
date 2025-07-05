#include "SliderWithAttachment.h"
#include "../Data/Messages.h"

SliderWithAttachment::SliderWithAttachment(
    juce::AudioProcessorValueTreeState& apvts, 
    const juce::String& id):
    apvts(apvts),
    isParameterChanging(false)
{
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, *this);
    setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
    setEnabled(true);
    setName(apvts.getParameter(id)->getName(1000));
}

void SliderWithAttachment::changeParameter(const juce::String& newId)
{
    isParameterChanging = true;
    attachment.reset();
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, newId, *this);
    setName(apvts.getParameter(newId)->getName(1000));
    isParameterChanging = false;
}

bool SliderWithAttachment::getIsParameterChanging()
{
    return isParameterChanging;
}
