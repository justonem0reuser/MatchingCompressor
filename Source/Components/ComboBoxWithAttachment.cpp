#include "ComboBoxWithAttachment.h"

ComboBoxWithAttachment::ComboBoxWithAttachment(
	juce::AudioProcessorValueTreeState& apvts, 
	const juce::String& id, 
	const juce::StringArray choices)
{
	auto name = apvts.getParameter(id)->getName(1000);
	setEditableText(false);
	setName(name);
	setEnabled(true);
	addItemList(choices, 1);
	attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, id, *this);
	setTitle(name);
}
