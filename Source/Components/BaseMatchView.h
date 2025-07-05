#pragma once
#include <JuceHeader.h>
#include "DataReceiver.h"

class BaseMatchView
{
public:
	std::function<void()> onOkButtonClicked;
	std::function<void()> onCancelButtonClicked;

	virtual juce::Component* getParent() = 0;
	virtual DataReceiver& getDataReceiver() = 0;
	virtual void setBusesConnected(bool mainBus, bool sidechain) = 0;
	virtual void setToggleButtonsUnchecked() = 0;
	virtual void setToggleButtonsDisabled() = 0;
	virtual void startTimer() = 0;
	virtual void timerCallback() = 0;
	virtual void stopTimer() = 0;
	virtual bool getMustBeInFront() const = 0;
	virtual void setMustBeInFront(bool mustBeInFront) = 0;
	virtual void catchException(const std::exception& e, juce::Component* parent) = 0;
};