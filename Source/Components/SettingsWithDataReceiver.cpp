#include "SettingsWithDataReceiver.h"
#include "../ComponentInitializerHelper.h"
#include "../ExceptionHelper.h"
#include "../Messages.h"
#include "../ParamsCalculator/CompParamsCalculatorNoEnv.h"
#include "../ParamsCalculator/CompParamsCalculatorEnv1D.h"
#include "../ParamsCalculator/CompParamsCalculatorEnv2D.h"

SettingsWithDataReceiver::SettingsWithDataReceiver(
	Component* parent, 
	juce::ValueTree& initProperties, 
	std::vector<ParameterInfo>& parameterInfos,
    bool isMainBusConnected, 
    bool isSidechainConnected,
    std::function<void(bool, bool)> toggleFunc):
	SettingsComponent(parent, initProperties, parameterInfos),
    dataReceiver(isMainBusConnected, isSidechainConnected, toggleFunc)
{
    dataReceiver.setBounds(0, 0, width, 130);
    addAndMakeVisible(dataReceiver);

    int currentY = dataReceiver.getBottom() + 2 * margin;
    panel.setBounds(margin, currentY, width - 2 * margin, panel.getTotalContentHeight());

    currentY = panel.getBottom() + 2 * margin;
    okButton.setButtonText(matchBtnStr);
    okButton.setBounds(margin, currentY, buttonWidth, componentHeight);
    cancelButton.setBounds(width - margin - buttonWidth, currentY, buttonWidth, componentHeight);

    currentY = okButton.getBottom() + margin;
    setSize(width, currentY);

    dataReceiver.onStateChanged = [this]
        {
            okButton.setEnabled(dataReceiver.isAllDataSet());
        };
    dataReceiver.onStateChanged();
}

std::vector<float>& SettingsWithDataReceiver::getResult()
{
    return compParams;
}

void SettingsWithDataReceiver::setBusesConnected(bool mainBus, bool sidechain)
{
    dataReceiver.setBusesConnected(mainBus, sidechain);
}

void SettingsWithDataReceiver::setToggleButtonsDisabled()
{
    dataReceiver.setToggleButtonsDisabled();
}

void SettingsWithDataReceiver::setToggleButtonsUnchecked()
{
    dataReceiver.setToggleButtonsUnchecked();
}

void SettingsWithDataReceiver::setFromDataCollector(
    std::vector<std::vector<float>>& refSamples,
    double refSampleRate,
    std::vector<std::vector<float>>& destSamples,
    double destSampleRate)
{
    dataReceiver.setFromDataCollector(refSamples, refSampleRate, destSamples, destSampleRate);
}

void SettingsWithDataReceiver::setOnTimer(std::function<void()> func)
{
    dataReceiver.onTimer = func;
}

void SettingsWithDataReceiver::startTimer()
{
    dataReceiver.startTimer(timerMs);
}

void SettingsWithDataReceiver::timerCallback()
{
    dataReceiver.timerCallback();
}

void SettingsWithDataReceiver::stopTimer()
{
    dataReceiver.stopTimer();
}

void SettingsWithDataReceiver::okButtonPressed()
{
    try
    {
        calculateCompressorParameters();
    }
    catch (const std::exception& e)
    {
        ExceptionHelper::catchException(e, this);
    }

    initProperties.copyPropertiesFrom(editedProperties, nullptr);
    parent->setVisible(false);
}

void SettingsWithDataReceiver::calculateCompressorParameters()
{
    // choosing an algorithm
    float attackMs = editedProperties.getProperty(setAttackId);
    float releaseMs = editedProperties.getProperty(setReleaseId);
    int channelAggregationTypeInt = editedProperties.getProperty(setChannelAggregationTypeId);
    int gainRegionsNumber = editedProperties.getProperty(setGainRegionsNumberId);

    std::vector<std::vector<float>> refSamples, destSamples;
    double refSampleRate, destSampleRate;
    dataReceiver.getReceivedData(refSamples, refSampleRate, destSamples, destSampleRate);

    std::unique_ptr<CompParamsCalculator> calculator;
    if (attackMs == 0 && releaseMs == 0 &&
        (destSamples.size() == 1 || channelAggregationTypeInt == 1))
        calculator.reset(new CompParamsCalculatorNoEnv());
    else if (destSamples[0].size() * destSamples.size() < gainRegionsNumber)
        calculator.reset(new CompParamsCalculatorEnv1D());
    else
        calculator.reset(new CompParamsCalculatorEnv2D());
    
    compParams = calculator->calculateCompressorParameters(
        refSamples, refSampleRate,
        destSamples, destSampleRate,
        editedProperties);
}

