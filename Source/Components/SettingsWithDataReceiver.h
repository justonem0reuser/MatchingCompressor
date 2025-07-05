#pragma once

#include <JuceHeader.h>
#include "SettingsComponent.h"
#include "DataReceiver.h"
#include "BaseMatchView.h"

class SettingsWithDataReceiver  : public SettingsComponent, public BaseMatchView
{ 
public:
	SettingsWithDataReceiver(
		Component* parent,
		juce::ValueTree& properties,
		std::vector<ParameterInfo>& parameterInfos,
		bool isMainBusConnected, 
		bool isSidechainConnected);
	void setBusesConnected(bool mainBus, bool sidechain) override;
	void setToggleButtonsDisabled() override;
	void setToggleButtonsUnchecked() override;
	void startTimer() override;
	void timerCallback() override;
	void stopTimer() override;
	juce::Component* getParent() override;
	DataReceiver& getDataReceiver();
	bool getMustBeInFront() const override;
	void setMustBeInFront(bool mustBeInFront) override;
	void catchException(const std::exception& e, juce::Component* parent) override;


private:
	const int timerMs = 50;
	const int buttonHeight = 40;

	DataReceiver dataReceiver;

	bool mustBeInFront;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsWithDataReceiver)
};
