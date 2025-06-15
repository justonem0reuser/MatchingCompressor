#include <JuceHeader.h>
#include "ToggleButtonWithAttachment.h"

//==============================================================================
ToggleButtonWithAttachment::ToggleButtonWithAttachment(
    juce::AudioProcessorValueTreeState& apvts, const juce::String& id) :
    attachment(apvts, id, *this)
{
    setEnabled(true);
    setName(id);
    setButtonText(apvts.getParameter(id)->getName(1000));
}