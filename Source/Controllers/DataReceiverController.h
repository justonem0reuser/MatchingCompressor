#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../Components/BaseDataReceiver.h"
#include "../Data/MatchingData.h"
#include "../Utilities/AudioFileReader.h"

class DataReceiverController
{
public:
	DataReceiverController(
		BaseDataReceiver* dataReceiver, 
		MatchingData& matchingData,
		MatchCompressorAudioProcessor& processor);
	void setFromDataCollector(
		std::vector<std::vector<float>>& refSamples,
		double refSampleRate,
		std::vector<std::vector<float>>& destSamples,
		double destSampleRate);
	void getReceivedData(
		std::vector<std::vector<float>>& refSamples,
		double& refSampleRate,
		std::vector<std::vector<float>>& destSamples,
		double& destSampleRate) const;

private:
	BaseDataReceiver* dataReceiver;
	MatchingData& matchingData;
	MatchCompressorAudioProcessor& processor;
	AudioFileReader audioFileReader;

	void getFromFile(juce::File& file, bool isRef);
	void checkAndSaveData(
		std::vector<std::vector<float>> samples,
		double sampleRate,
		bool isFile,
		bool isRef);
};