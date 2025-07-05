#pragma once
#include <JuceHeader.h>
#include "BaseDataReceiver.h"
#include "../Data/MatchingData.h"

class DataReceiver : 
	public juce::Component, 
	public BaseDataReceiver,
	public juce::Timer
{
public:
	std::function<void()> TimerTicked;

	DataReceiver(
		bool isMainBusConnected,
		bool isSidechainConnected);

	void setRefDataState(SetDataState state) override;
	void setDestDataState(SetDataState state) override;
	bool isReadRefFromStreamEnabled() override;
	bool isReadDestFromStreamEnabled() override;
	void setNeedChangeRefLabelText() override;
	void setNeedChangeDestLabelText() override;
	void setBusesConnected(bool mainBus, bool sidechain) override;

	bool isAllDataSet() const;
	void setToggleButtonsDisabled();
	void setToggleButtonsUnchecked();
	void timerCallback() override;
	void resized() override;

private:
	const juce::String filePatterns = "*.aif;*.aiff;*.flac;*.ogg;*.wav;*.wma";

	const int buttonHeight = 40;
	const int labelHeight = 10;
	const int margin = 20;

	juce::TextButton openRefFileButton, openDestFileButton;
	juce::TextButton readRefFromStreamButton, readDestFromStreamButton;
	juce::Label refLabel, destLabel;
	juce::Label refIsSetLabel, destIsSetLabel;

	std::unique_ptr<juce::FileChooser> chooser;
	std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

	bool refEnabled, destEnabled;
	bool refChecked = false, destChecked = false;
	bool needChangeToggleButtonsState = true,
		needChangeRefLabelText = false,
		needChangeDestLabelText = false;
	bool isRefDataSet = false, isDestDataSet = false;
	juce::String refToggleButtonText, destToggleButtonText;

	void openFile(bool isRef);
};