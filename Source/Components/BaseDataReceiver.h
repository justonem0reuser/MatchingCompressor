#pragma once
#include <JuceHeader.h>
#include "../Utilities/SetDataState.h"

class BaseDataReceiver
{
public:
	std::function<void(juce::File& file, bool isRef)> onFileChosen;
	std::function<void()> onRefReceivedFromBus;
	std::function<void()> onDestReceivedFromBus;
	std::function<void()> onStateChanged;
	std::function<void(bool, bool)> onCollectFromBusStateChanged;

	virtual void setRefDataState(SetDataState state) = 0;
	virtual void setDestDataState(SetDataState state) = 0;
	virtual bool isReadRefFromStreamEnabled() = 0;
	virtual bool isReadDestFromStreamEnabled() = 0;
	virtual void setNeedChangeRefLabelText() = 0;
	virtual void setNeedChangeDestLabelText() = 0;
	virtual void setBusesConnected(bool mainBus, bool sidechain) = 0;
};