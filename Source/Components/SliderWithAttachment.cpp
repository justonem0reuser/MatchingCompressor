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
    setNameFromParameter(apvts.getParameter(id));
}

void SliderWithAttachment::changeParameter(const juce::String& newId)
{
    isParameterChanging = true;
    attachment.reset();
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, newId, *this);
    setNameFromParameter(apvts.getParameter(newId));
    isParameterChanging = false;
}

bool SliderWithAttachment::getIsParameterChanging()
{
    return isParameterChanging;
}

void SliderWithAttachment::setNameFromParameter(juce::RangedAudioParameter* parameter)
{
    auto name = parameter->getName(1000);
    auto label = parameter->getLabel();
    if (label.isNotEmpty())
        name += " (" + label + ")";
    setName(name);
}
