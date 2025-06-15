#pragma once
#include <JuceHeader.h>

class DataReceiver : public juce::Component, public juce::Timer
{
public:
	std::function<void()> onStateChanged;
	std::function<void()> onTimer;
	
	DataReceiver(
		bool isMainBusConnected,
		bool isSidechainConnected,
		std::function<void(bool, bool)> toggleFunc);
	bool isAllDataSet() const;
	void getReceivedData(
		std::vector<std::vector<float>>& refSamples,
		double& refSampleRate,
		std::vector<std::vector<float>>& destSamples,
		double& destSampleRate) const;
	void setBusesConnected(bool mainBus, bool sidechain);
	void setToggleButtonsDisabled();
	void setToggleButtonsUnchecked();
	void setFromDataCollector(
		std::vector<std::vector<float>>& refSamples,
		double refSampleRate,
		std::vector<std::vector<float>>& destSamples,
		double destSampleRate);
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
	juce::AudioFormatManager formatManager;
	std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

	std::vector<std::vector<float>> refSamples, destSamples;
	std::vector<std::vector<float>> newRefSamples, newDestSamples;
	double refSampleRate = 0, destSampleRate = 0;
	double newRefSampleRate = 0, newDestSampleRate = 0;

	std::function<void(bool, bool)> mToggleFuncBool;
	bool refEnabled, destEnabled;
	bool refChecked = false, destChecked = false;
	bool needChangeToggleButtonsState = true,
		needChangeRefLabelText = false,
		needChangeDestLabelText = false;
	bool isRefDataSet = false, isDestDataSet = false;
	juce::String refToggleButtonText, destToggleButtonText;

	void openFile(bool isRef);
	void readFromFile(
		juce::File file,
		bool excludeZeroSamples,
		std::vector<std::vector<float>>& res,
		double& sampleRate);
	static void checkSourceDestFileData(
		std::vector<std::vector<float>> samples,
		double sampleRate,
		bool isFile,
		bool isRef);
};