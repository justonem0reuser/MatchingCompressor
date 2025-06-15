#pragma once
#include <JuceHeader.h>
#include "Components/SettingsWithDataReceiver.h"

class MatchWindow : public juce::DocumentWindow
{
public:
	MatchWindow(
		juce::ValueTree& properties,
		std::vector<ParameterInfo>& parameterInfos,
		bool isMainBusConnected, 
		bool isSidechainConnected,
		std::function<void(bool, bool)> toggleFunc);
	std::vector<float>& getResult();
	bool getMustBeInFront();
	void setMustBeInFront(bool mustBeInFront);


	// redirections to component
	void resetLookAndFeel();
	void setFromDataCollector(
		std::vector<std::vector<float>>& refSamples,
		double refSampleRate,
		std::vector<std::vector<float>>& destSamples,
		double destSampleRate);
	void setBusesConnected(bool mainBus, bool sidechain);
	void setToggleButtonsDisabled();
	void setToggleButtonsUnchecked();
	void setOnTimer(std::function<void()> func);
	void startTimer();
	void stopTimer();
	void timerCallback();

private:
	SettingsWithDataReceiver component;
	bool mustBeInFront;

	void closeButtonPressed() override;
};
