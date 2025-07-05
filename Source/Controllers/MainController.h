#pragma once
#include "MatchController.h"
#include "../Components/BaseMainView.h"
#include "../PluginProcessor.h"

class MainController:
	public juce::ComponentListener
{
public:
	MainController(
		BaseMainView* editor,
		MatchCompressorAudioProcessor& processor);
	MatchController& getMatchController();
private:
	MatchController matchController;

	MatchingData& matchingData;

	BaseMainView* editor;
	MatchCompressorAudioProcessor& processor;

	void onToolButtonClicked();
	void onMatchViewClosed();
	void onPrepareToPlay();
	void onResetButtonClicked();
	
	void endCollectingData(bool saveData, bool resetButtonsState);
	
	void setInactiveKneesParameters();

	void setParameter(
		juce::String name, 
		const juce::NormalisableRange<float> range,
		float value);
};