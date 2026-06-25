#include "MatchController.h"
#include "../ParamsCalculator/CompParamsCalculatorFactory.h"

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
    std::vector<std::vector<float>> refSamples, destSamples;
    double refSampleRate, destSampleRate;
    dataReceiverController.getReceivedData(refSamples, refSampleRate, destSamples, destSampleRate);
    auto calculator = CompParamsCalculatorFactory::create(destSamples, matchingData.properties);
    matchingData.calculatedCompParams = calculator->calculateCompressorParameters(
        refSamples, refSampleRate,
        destSamples, destSampleRate,
        matchingData.properties);
    juce::NullCheckedInvocation::invoke(CompParamsCalculated);
}
