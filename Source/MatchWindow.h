#pragma once
#include <JuceHeader.h>
#include "Components/SettingsWithDataReceiver.h"
#include "Components/BaseMatchView.h"

class MatchWindow : public juce::DocumentWindow
{
public:
	MatchWindow(
		juce::ValueTree& properties,
		std::vector<ParameterInfo>& parameterInfos,
		bool isMainBusConnected, 
		bool isSidechainConnected);

	BaseMatchView* getMatchView();
	void resetLookAndFeel();

private:
	SettingsWithDataReceiver component;

	void closeButtonPressed() override;
};
