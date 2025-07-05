#pragma once
#include <JuceHeader.h>
#include "../Data/ParameterInfo.h"

class SettingsComponent : public juce::Component
{
public:
	SettingsComponent(
		juce::Component* parent,
		juce::ValueTree& properties,
		std::vector<ParameterInfo>& parameterInfos);
	~SettingsComponent();

	void resetLookAndFeel();

protected:
	const int width = 550;
	const int margin = 20;
	const int buttonWidth = (width - 3 * margin) / 2;
	const int componentHeight = 30;

	juce::Component* parent;

	juce::TextButton okButton, cancelButton;

	juce::PropertyPanel panel;

	juce::ValueTree& properties;

private:
	juce::Array<juce::PropertyComponent*> propComps;
	juce::Array<juce::PropertyComponent*> createPropertyComponents(
		std::vector<ParameterInfo>& parameterInfos);
};