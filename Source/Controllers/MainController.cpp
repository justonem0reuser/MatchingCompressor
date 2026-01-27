#include "MainController.h"
#include "../Data/Messages.h"
#include "../Data/Ranges.h"

MainController::MainController(
	BaseMainView* editor, 
	MatchCompressorAudioProcessor& processor):
	editor(editor),
	processor(processor),
	matchingData(processor.getMatchingData()),
	matchController(editor->getMatchView(), processor.getMatchingData(), processor)
{
    matchController.MatchViewClosed = [this] { onMatchViewClosed(); };
    processor.PrepareToPlay = [this] { onPrepareToPlay(); };
    editor->ResetButtonClicked = [this] { onResetButtonClicked(); };
    editor->ToolButtonClicked = [this] { onToolButtonClicked(); };
    editor->BoundsChanged = [this] { setInactiveKneesParameters(); };
}

MatchController& MainController::getMatchController()
{
    return matchController;
}

void MainController::onToolButtonClicked()
{
    BaseMatchView* matchView = this->editor->getMatchView();
    matchView->setBusesConnected(
        processor.isInputBusConnected(0),
        processor.isInputBusConnected(1));
    if (processor.isPlayHeadPlaying())
        matchView->setToggleButtonsDisabled();

    processor.PlayHeadStartPlaying = [matchView]
        {
            matchView->setToggleButtonsDisabled();
            matchView->setMustBeInFront(true);
        };
    processor.PlayHeadStopPlaying = [this, matchView]
        {
            endCollectingData(true, true);
            matchView->setMustBeInFront(true);
        };
    processor.DataCollectorMemoryFull = [this, matchView]
        {
            endCollectingData(true, false);
            matchView->setMustBeInFront(true);
        };
    matchView->getDataReceiver().TimerTicked = [matchView]
        {
            if (matchView->getMustBeInFront())
            {
                matchView->getParent()->toFront(false);
                matchView->setMustBeInFront(false);
            }
        };
    matchView->startTimer();
}

void MainController::onMatchViewClosed()
{
    BaseMatchView* matchView = editor->getMatchView();
    matchView->stopTimer();
    endCollectingData(false, true);
    matchView->timerCallback();
    matchView->getDataReceiver().TimerTicked = NULL;
    processor.PlayHeadStartPlaying = NULL;
    processor.PlayHeadStopPlaying = NULL;
    processor.DataCollectorMemoryFull = NULL;
}

void MainController::onPrepareToPlay()
{
    editor->getMatchView()->setBusesConnected(
        processor.isInputBusConnected(0),
        processor.isInputBusConnected(1));
}

void MainController::onResetButtonClicked()
{
    int kneesNumber = matchingData.properties.getProperty(setKneesNumberId);
    int chAggrType = matchingData.properties.getProperty(setChannelAggregationTypeId);
    int filterType = matchingData.properties.getProperty(setBalFilterTypeId);
    float attackMs = matchingData.properties.getProperty(setAttackId);
    float releaseMs = matchingData.properties.getProperty(setReleaseId);
    float gain = matchingData.calculatedCompParams[0];

    setParameter(kneesNumberId, kneesNumberRange, kneesNumber);
    setParameter(channelAggrerationTypeId, channelAggregationTypeRange, chAggrType);
    setParameter(balFilterTypeId, envelopeTypeRange, filterType);
    setParameter(attackId, attackRange, attackMs);
    setParameter(releaseId, releaseRange, releaseMs);
    setParameter(gainId, gainRange, gain);

    for (int i = 0; i < DynamicShaper<float>::maxKneesNumber; i++)
    {
        float t, r, kw;
        if (i < kneesNumber)
        {
            t = matchingData.calculatedCompParams[i * 3 + 1];
            r = matchingData.calculatedCompParams[i * 3 + 2];
            kw = matchingData.calculatedCompParams[i * 3 + 3];
        }
        else
        {
            t = kw = 0.f;
            r = matchingData.calculatedCompParams[2];
        }
        if (r < 1)
            r = 2.f - 1.f / r;

        auto iStr = std::to_string(i);
        setParameter(thresholdId + iStr, thresholdRange, t);
        setParameter(ratioId + iStr, ratioRange, r);
        setParameter(kneeWidthId + iStr, kneeWidthRange, kw);
    }
}

void MainController::endCollectingData(bool saveData, bool resetButtonsState)
{
    std::vector<std::vector<float>> mainBusData, sideChainData;
    double mainBusRate, sidechainRate;
    processor.getCollectedData(mainBusData, mainBusRate, sideChainData, sidechainRate);
    if (saveData)
        matchController
            .getDataReceiverController()
            .setFromDataCollector(sideChainData, sidechainRate, mainBusData, mainBusRate);
    if (resetButtonsState)
    {
        editor->getMatchView()->setBusesConnected(
            processor.isInputBusConnected(0),
            processor.isInputBusConnected(1));
        editor->getMatchView()->setToggleButtonsUnchecked();
    }
}

void MainController::setInactiveKneesParameters()
{
    auto& apvts = processor.apvts;
    int kneesNumber = *apvts.getRawParameterValue(kneesNumberId);
    const int maxKneesNumber = DynamicShaper<float>::maxKneesNumber;

    for (int i = kneesNumber; i < maxKneesNumber; i++)
    {
        auto prevStr = std::to_string(i - 1);
        float prevThreshold =
            *apvts.getRawParameterValue(thresholdId + prevStr);
        float prevKneeWidth =
            *apvts.getRawParameterValue(kneeWidthId + prevStr);
        float leftBound = std::min(prevThreshold + 0.5f * prevKneeWidth, 0.f);

        auto curStr = std::to_string(i);
        float currentThreshold = *apvts.getRawParameterValue(thresholdId + curStr);
        if (currentThreshold <= leftBound)
        {
            setParameter(thresholdId + curStr, thresholdRange, leftBound);
            setParameter(kneeWidthId + curStr, kneeWidthRange, 0.f);
        }
        else
        {
            float currentKneeWidth = *apvts.getRawParameterValue(kneeWidthId + curStr);
            float delta = currentThreshold - leftBound;
            if (i < maxKneesNumber - 1)
                delta = std::min(delta, -currentThreshold);
            if (0.5f * currentKneeWidth >= delta)
                setParameter(kneeWidthId + curStr, kneeWidthRange, 2.f * delta);
        }
    }
}

void MainController::setParameter(
    juce::String name,
    const juce::NormalisableRange<float> range,
    float value)
{
    auto* par = processor.apvts.getParameter(name);
    par->beginChangeGesture();
    par->setValueNotifyingHost(range.convertTo0to1(value));
    par->endChangeGesture();
}
