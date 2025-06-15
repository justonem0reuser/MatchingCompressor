#pragma once
#include <JuceHeader.h>
#include "../ParameterInfo.h"

class SettingsComponent : public juce::Component
{
public:
	SettingsComponent(
		Component* parent,
		juce::ValueTree& initProperties,
		std::vector<ParameterInfo>& parameterInfos);
	~SettingsComponent();

	void resetLookAndFeel();
	void cancelButtonPressed();

protected:
	const int width = 550;
	const int margin = 20;
	const int buttonWidth = (width - 3 * margin) / 2;
	const int componentHeight = 30;

	Component* parent;

	juce::PropertyPanel panel;
	juce::TextButton okButton, cancelButton;

	juce::Array<juce::PropertyComponent*> propComps;

	juce::ValueTree initProperties;
	juce::ValueTree editedProperties;

	void virtual okButtonPressed();
	juce::Array<juce::PropertyComponent*> createPropertyComponents(
		juce::ValueTree& initProperties,
		std::vector<ParameterInfo>& parameterInfos);
};