#pragma once
#include <JuceHeader.h>

class SliderWithAttachment : public juce::Slider
{
public:
	SliderWithAttachment(juce::AudioProcessorValueTreeState& apvts, const juce::String &id);
	void changeParameter(const juce::String& newId);
	bool getIsParameterChanging();

private:
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
	juce::AudioProcessorValueTreeState& apvts;
	bool isParameterChanging;

	void setNameFromParameter(juce::RangedAudioParameter* parameter);
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderWithAttachment)
};