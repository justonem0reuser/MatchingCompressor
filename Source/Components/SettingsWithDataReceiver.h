#pragma once

#include <JuceHeader.h>
#include "SettingsComponent.h"
#include "DataReceiver.h"

class SettingsWithDataReceiver  : public SettingsComponent
{ 
public:
	SettingsWithDataReceiver(
		Component* parent,
		juce::ValueTree& initProperties,
		std::vector<ParameterInfo>& parameterInfos,
		bool isMainBusConnected, 
		bool isSidechainConnected,
		std::function<void(bool, bool)> toggleFunc);
	std::vector<float>& getResult();
	void setBusesConnected(bool mainBus, bool sidechain);
	void setToggleButtonsDisabled();
	void setToggleButtonsUnchecked();
	void setFromDataCollector(
		std::vector<std::vector<float>>& refSamples,
		double refSampleRate,
		std::vector<std::vector<float>>& destSamples,
		double destSampleRate);
	void setOnTimer(std::function<void()> func);
	void startTimer();
	void timerCallback();
	void stopTimer();

private:
	const int timerMs = 50;
	const int buttonHeight = 40;

	DataReceiver dataReceiver;

	std::vector<float> refStat, destStat, compParams;

	void okButtonPressed() override;
	void calculateCompressorParameters();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsWithDataReceiver)
};
