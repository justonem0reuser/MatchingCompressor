#pragma once
#include "../Components/BaseMatchView.h"
#include "../Data/MatchingData.h"
#include "DataReceiverController.h"

/// <summary>
/// Match window component controller
/// </summary>
class MatchController
{
public:
	std::function<void()> CompParamsCalculated;
	std::function<void()> MatchViewClosed;

	MatchController(
		BaseMatchView* matchView, 
		MatchingData& matchingData,
		MatchCompressorAudioProcessor& processor);
	DataReceiverController& getDataReceiverController();

	static std::vector<float> getCurrentCompParams(
		juce::AudioProcessorValueTreeState& apvts,
		int kneesNumber);

private:
	BaseMatchView* matchView;
	MatchingData& matchingData;
	MatchCompressorAudioProcessor& processor;
	DataReceiverController dataReceiverController;

	void calculateCompressorParameters();

	void closeMatchWindow();

	void syncPropertiesFromState();
};