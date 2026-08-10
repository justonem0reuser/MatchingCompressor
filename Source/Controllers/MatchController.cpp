#include "MatchController.h"
#include "../ParamsCalculator/CompParamsCalculatorFactory.h"

MatchController::MatchController(
	BaseMatchView* matchView, 
	MatchingData& matchingData,
    MatchCompressorAudioProcessor& processor):
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
                dynamic_cast<juce::Component*>(this->matchView)->getParentComponent()->setVisible(false);
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
            dynamic_cast<juce::Component*>(this->matchView)->getParentComponent()->setVisible(false);
            juce::NullCheckedInvocation::invoke(MatchViewClosed);
        };
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

    auto calculator = CompParamsCalculatorFactory::create(destSamples, matchingData.properties);
    matchingData.calculatedCompParams = calculator->calculateCompressorParameters(
        refSamples,
        destSamples, matchingData.destSampleRate,
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
