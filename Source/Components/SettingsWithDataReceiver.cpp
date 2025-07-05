#include "SettingsWithDataReceiver.h"
#include "ComponentInitializerHelper.h"
#include "../Utilities/ExceptionHelper.h"
#include "../Data/Messages.h"

SettingsWithDataReceiver::SettingsWithDataReceiver(
    Component* parent,
    juce::ValueTree& properties,
    std::vector<ParameterInfo>& parameterInfos,
    bool isMainBusConnected,
    bool isSidechainConnected) :
    SettingsComponent(parent, properties, parameterInfos),
    dataReceiver(isMainBusConnected, isSidechainConnected),
    mustBeInFront(false)
{
    dataReceiver.setBounds(0, 0, width, 130);
    addAndMakeVisible(dataReceiver);

    int currentY = dataReceiver.getBottom() + 2 * margin;
    panel.setBounds(margin, currentY, width - 2 * margin, panel.getTotalContentHeight());

    currentY = panel.getBottom() + 2 * margin;
    okButton.setButtonText(matchBtnStr);
    okButton.setBounds(margin, currentY, buttonWidth, componentHeight);
    okButton.onClick = [this] { juce::NullCheckedInvocation::invoke(onOkButtonClicked); };
    cancelButton.setBounds(width - margin - buttonWidth, currentY, buttonWidth, componentHeight);
    cancelButton.onClick = [this] { juce::NullCheckedInvocation::invoke(onCancelButtonClicked); };

    currentY = okButton.getBottom() + margin;
    setSize(width, currentY);

    dataReceiver.onStateChanged = [this]
        {
            okButton.setEnabled(dataReceiver.isAllDataSet());
        };
    dataReceiver.onStateChanged();
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

juce::Component* SettingsWithDataReceiver::getParent()
{
    return parent;
}

DataReceiver& SettingsWithDataReceiver::getDataReceiver()
{
    return dataReceiver;
}

bool SettingsWithDataReceiver::getMustBeInFront() const
{
    return mustBeInFront;
}

void SettingsWithDataReceiver::setMustBeInFront(bool mustBeInFront)
{
    this->mustBeInFront = mustBeInFront;
}

void SettingsWithDataReceiver::catchException(const std::exception& e, juce::Component* parent)
{
    ExceptionHelper::catchException(e, parent);
}

