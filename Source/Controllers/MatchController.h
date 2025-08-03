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

private:
	BaseMatchView* matchView;
	MatchingData& matchingData;
	DataReceiverController dataReceiverController;

	void calculateCompressorParameters();
};