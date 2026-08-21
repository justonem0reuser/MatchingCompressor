#include "MatchController.h"
#include "../ParamsCalculator/CompParamsCalculatorFactory.h"

MatchController::MatchController(
    BaseMatchView* matchView,
    MatchingData& matchingData,
    MatchCompressorAudioProcessor& processor) :
    matchView(matchView),
    matchingData(matchingData),
    processor(processor),
    dataReceiverController(&matchView->getDataReceiver(), matchingData, processor)
{
    this->matchView->onOkButtonClicked = [this]
        {
            try
            {
                calculateCompressorParameters();
                this->matchingData.initProperties.copyPropertiesFrom(this->matchingData.properties, nullptr);
                closeMatchWindow();
                juce::NullCheckedInvocation::invoke(MatchViewClosed);
            }
            catch (const std::exception& e)
            {
                this->matchView->catchException(e, dynamic_cast<juce::Component*>(this->matchView));
            }
        };
    this->matchView->onCancelButtonClicked =
        [this]
        {
            this->matchingData.properties.copyPropertiesFrom(this->matchingData.initProperties, nullptr);
            closeMatchWindow();
            juce::NullCheckedInvocation::invoke(MatchViewClosed);
        };
}

void MatchController::closeMatchWindow()
{
    auto* window = dynamic_cast<juce::Component*>(matchView)->getParentComponent();
    window->exitModalState(0);
    window->setVisible(false);
}

DataReceiverController& MatchController::getDataReceiverController()
{
    return dataReceiverController;
}

void MatchController::calculateCompressorParameters()
{
    auto& refSamples = matchingData.refSamples;
    auto& destSamples = matchingData.destSamples;

    if (refSamples.empty() || refSamples[0].empty())
    {

        syncPropertiesFromState();
        int kneesNumber = matchingData.properties.getProperty(setKneesNumberId);
        matchingData.calculatedCompParams = getCurrentCompParams(processor.apvts, kneesNumber);
        matchingData.matchedWithReference = false;
        juce::NullCheckedInvocation::invoke(CompParamsCalculated);
        return;
    }

    bool isKWeightingUsed = CompParamsCalculator::isKWeightingUsed(matchingData.properties);
    std::vector<std::vector<float>> weightedRef, weightedDest;
    if (isKWeightingUsed)
    {
        weightedRef = CompParamsCalculator::applyKWeighting(refSamples, matchingData.refSampleRate);
        weightedDest = CompParamsCalculator::applyKWeighting(destSamples, matchingData.destSampleRate);
    }
    auto& matchingRef = isKWeightingUsed ? weightedRef : refSamples;
    auto& matchingDest = isKWeightingUsed ? weightedDest : destSamples;

    auto calculator = CompParamsCalculatorFactory::create(matchingDest, matchingData.properties);
    matchingData.calculatedCompParams = calculator->calculateCompressorParameters(
        matchingRef,
        matchingDest, matchingData.destSampleRate,
        matchingData.properties);
    matchingData.fitMismatch = calculator->getLastFitMismatch();
    matchingData.matchedWithReference = true;
    juce::NullCheckedInvocation::invoke(CompParamsCalculated);
}

void MatchController::syncPropertiesFromState()
{
    auto& apvts = processor.apvts;
    auto& p = matchingData.properties;
    p.setProperty(setKneesNumberId,
        juce::roundToInt(apvts.getRawParameterValue(kneesNumberId)->load()), nullptr);
    p.setProperty(setBalFilterTypeId,
        juce::roundToInt(apvts.getRawParameterValue(balFilterTypeId)->load()), nullptr);
    p.setProperty(setChannelAggregationTypeId,
        juce::roundToInt(apvts.getRawParameterValue(channelAggrerationTypeId)->load()), nullptr);
    p.setProperty(setAttackId, apvts.getRawParameterValue(attackId)->load(), nullptr);
    p.setProperty(setReleaseId, apvts.getRawParameterValue(releaseId)->load(), nullptr);
}

std::vector<float> MatchController::getCurrentCompParams(
    juce::AudioProcessorValueTreeState& apvts,
    int kneesNumber)
{
    std::vector<float> params(1 + 3 * kneesNumber);
    params[0] = *apvts.getRawParameterValue(gainId);
    for (int i = 0; i < kneesNumber; i++)
    {
        auto iStr = std::to_string(i);
        float thr = *apvts.getRawParameterValue(thresholdId + iStr);
        float ratioInverse = *apvts.getRawParameterValue(ratioInverseId + iStr);
        float kw = *apvts.getRawParameterValue(kneeWidthId + iStr);
        params[1 + 3 * i] = thr;
        params[2 + 3 * i] = ratioInverse;
        params[3 + 3 * i] = kw;
    }
    return params;
}
