#include "MatchController.h"
#include "../ParamsCalculator/CompParamsCalculatorNoEnv.h"
#include "../ParamsCalculator/CompParamsCalculatorEnv1D.h"
#include "../ParamsCalculator/CompParamsCalculatorEnv2D.h"
#include "../Data/Messages.h"

MatchController::MatchController(
	BaseMatchView* matchView, 
	MatchingData& matchingData,
    MatchCompressorAudioProcessor& processor):
	matchView(matchView),
	matchingData(matchingData),
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
    // choosing an algorithm
    float attackMs = matchingData.properties.getProperty(setAttackId);
    float releaseMs = matchingData.properties.getProperty(setReleaseId);
    int channelAggregationTypeInt = matchingData.properties.getProperty(setChannelAggregationTypeId);
    int gainRegionsNumber = matchingData.properties.getProperty(setGainRegionsNumberId);

    std::vector<std::vector<float>> refSamples, destSamples;
    double refSampleRate, destSampleRate;
    dataReceiverController.getReceivedData(refSamples, refSampleRate, destSamples, destSampleRate);

    std::unique_ptr<CompParamsCalculator> calculator;
    if (attackMs == 0 && releaseMs == 0 &&
        (destSamples.size() == 1 || channelAggregationTypeInt == 1))
        calculator.reset(new CompParamsCalculatorNoEnv());
    else if (destSamples[0].size() * destSamples.size() < gainRegionsNumber)
        calculator.reset(new CompParamsCalculatorEnv1D());
    else
        calculator.reset(new CompParamsCalculatorEnv2D());

    matchingData.calculatedCompParams = calculator->calculateCompressorParameters(
        refSamples, refSampleRate,
        destSamples, destSampleRate,
        matchingData.properties);
    juce::NullCheckedInvocation::invoke(CompParamsCalculated);
}
