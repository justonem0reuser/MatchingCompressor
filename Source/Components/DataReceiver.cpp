#include "DataReceiver.h"
#include "../Data/Messages.h"
#include "../Utilities/ExceptionHelper.h"
#include "ComponentInitializerHelper.h"

DataReceiver::DataReceiver(
    bool isMainBusConnected, bool 
    isSidechainConnected):
    refEnabled(isSidechainConnected),
    destEnabled(isMainBusConnected),
    refToggleButtonText(isSidechainConnected ? readFromSidechainBusStr : sidechainBusIsDisconnectedStr),
    destToggleButtonText(isMainBusConnected ? readFromMainBusStr : mainBusIsDisconnectedStr)
{
    auto refToggleFunc = [this] {
        refChecked = readRefFromStreamButton.getToggleState();
        juce::NullCheckedInvocation::invoke(
            onCollectFromBusStateChanged,
            readDestFromStreamButton.getToggleState(),
            readRefFromStreamButton.getToggleState());
        };
    auto destToggleFunc = [this] {
        destChecked = readDestFromStreamButton.getToggleState();
        juce::NullCheckedInvocation::invoke(
            onCollectFromBusStateChanged,
            readDestFromStreamButton.getToggleState(),
            readRefFromStreamButton.getToggleState());
        };

    ComponentInitializerHelper::initLabel(this, refLabel);
    ComponentInitializerHelper::initLabel(this, destLabel);

    ComponentInitializerHelper::initTextButton(this, openRefFileButton, openFromFileStr, [this] { openFile(true); });
    ComponentInitializerHelper::initTextButton(this, openDestFileButton, openFromFileStr, [this] { openFile(false); });

    ComponentInitializerHelper::initTextButton(
        this, readRefFromStreamButton, refToggleButtonText, refToggleFunc);
    ComponentInitializerHelper::initTextButton(
        this, readDestFromStreamButton, destToggleButtonText, destToggleFunc);
    readRefFromStreamButton.setClickingTogglesState(true);
    readDestFromStreamButton.setClickingTogglesState(true);

    ComponentInitializerHelper::initLabel(this, refIsSetLabel);
    ComponentInitializerHelper::initLabel(this, destIsSetLabel);

    refLabel.setJustificationType(juce::Justification::centred);
    destLabel.setJustificationType(juce::Justification::centred);
    refIsSetLabel.setJustificationType(juce::Justification::centred);
    destIsSetLabel.setJustificationType(juce::Justification::centred);
    refLabel.setText(refAudioStr, juce::NotificationType::dontSendNotification);
    destLabel.setText(destAudioStr, juce::NotificationType::dontSendNotification);
}

void DataReceiver::setRefDataState(SetDataState state)
{
    isRefDataSet = state != SetDataState::NotSet;
    refIsSetLabel.setText(
        state == SetDataState::SetFromBus ? refSetFromBusStr :
        state == SetDataState::SetFromFile ? refSetFromFileStr :
        "",
        juce::NotificationType::dontSendNotification);
    refIsSetLabel.repaint();
}

void DataReceiver::setDestDataState(SetDataState state)
{
    isDestDataSet = state != SetDataState::NotSet;
    destIsSetLabel.setText(
        state == SetDataState::SetFromBus ? destSetFromBusStr :
        state == SetDataState::SetFromFile ? destSetFromFileStr :
        "",
        juce::NotificationType::dontSendNotification);
    destIsSetLabel.repaint();
}

bool DataReceiver::isReadRefFromStreamEnabled()
{
    return readRefFromStreamButton.getToggleState();
}

bool DataReceiver::isReadDestFromStreamEnabled()
{
    return readDestFromStreamButton.getToggleState();
}

void DataReceiver::setNeedChangeRefLabelText()
{
    needChangeRefLabelText = true;
}

void DataReceiver::setNeedChangeDestLabelText()
{
    needChangeDestLabelText = true;
}

bool DataReceiver::isAllDataSet() const
{
    return isRefDataSet && isDestDataSet;
}

void DataReceiver::setBusesConnected(bool mainBus, bool sidechain)
{
    refEnabled = sidechain;
    destEnabled = mainBus;
    if (!sidechain)
        refChecked = false;
    if (!mainBus)
        destChecked = false;
    refToggleButtonText = sidechain ? readFromSidechainBusStr : sidechainBusIsDisconnectedStr;
    destToggleButtonText = mainBus ? readFromMainBusStr : mainBusIsDisconnectedStr;
    needChangeToggleButtonsState = true;
}

void DataReceiver::setToggleButtonsDisabled()
{
    refEnabled = destEnabled = false;
    needChangeToggleButtonsState = true;
}

void DataReceiver::setToggleButtonsUnchecked()
{
    if (refChecked)
    {
        refChecked = false;
        needChangeToggleButtonsState = true;
    }
    if (destChecked)
    {
        destChecked = false;
        needChangeToggleButtonsState = true;
    }
}

void DataReceiver::timerCallback()
{
    if (needChangeToggleButtonsState)
    {
        readRefFromStreamButton.setButtonText(refToggleButtonText);
        readDestFromStreamButton.setButtonText(destToggleButtonText);
        if (readRefFromStreamButton.getToggleState() != refChecked)
            readRefFromStreamButton.triggerClick();
        if (readDestFromStreamButton.getToggleState() != destChecked)
            readDestFromStreamButton.triggerClick();
        readRefFromStreamButton.setEnabled(refEnabled);
        readDestFromStreamButton.setEnabled(destEnabled);
        needChangeToggleButtonsState = false;
    }
    try
    {
        if (needChangeRefLabelText)
        {
            juce::NullCheckedInvocation::invoke(onRefReceivedFromBus);
            setRefDataState(SetDataState::SetFromBus);
            juce::NullCheckedInvocation::invoke(onStateChanged);

        }
        if (needChangeDestLabelText)
        {
            juce::NullCheckedInvocation::invoke(onDestReceivedFromBus);
            setDestDataState(SetDataState::SetFromBus);
            juce::NullCheckedInvocation::invoke(onStateChanged);
        }
    }
    catch (const std::exception& e)
    {
        ExceptionHelper::catchException(e, this);
    }
    needChangeRefLabelText = false;
    needChangeDestLabelText = false;

    juce::NullCheckedInvocation::invoke(TimerTicked);
}

void DataReceiver::resized()
{
    int width = getBounds().getWidth();
    const int buttonWidth = (width - 3 * margin) / 2;
    const int rightLabelX = (width + margin) / 2;
    int currentY = margin;
    refLabel.setBounds(margin, currentY, buttonWidth, labelHeight);
    destLabel.setBounds(rightLabelX, currentY, buttonWidth, labelHeight);

    currentY = refLabel.getBottom() + margin;
    openRefFileButton.setBounds(margin, currentY, buttonWidth / 2, buttonHeight);
    readRefFromStreamButton.setBounds(openRefFileButton.getRight(), currentY, buttonWidth / 2, buttonHeight);
    openDestFileButton.setBounds(rightLabelX, currentY, buttonWidth / 2, buttonHeight);
    readDestFromStreamButton.setBounds(openDestFileButton.getRight(), currentY, buttonWidth / 2, buttonHeight);

    currentY = openRefFileButton.getBottom() + margin;
    refIsSetLabel.setBounds(margin, currentY, buttonWidth, labelHeight);
    destIsSetLabel.setBounds(rightLabelX, currentY, buttonWidth, labelHeight);
}

void DataReceiver::openFile(bool isRef)
{
    chooser = std::make_unique<juce::FileChooser>(
        fileChooserTitleStr,
        juce::File{},
        filePatterns);
    auto chooserFlags = juce::FileBrowserComponent::FileChooserFlags::openMode
        | juce::FileBrowserComponent::FileChooserFlags::canSelectFiles;
    std::function<void(const juce::FileChooser& fc)> function =
        [this, isRef](const juce::FileChooser& fc)
        {
            try
            {
                auto file = fc.getResult();
                if (file != juce::File{})
                    juce::NullCheckedInvocation::invoke(onFileChosen, file, isRef);
            }
            catch (const std::exception& e)
            {
                ExceptionHelper::catchException(e, this);
            }
            juce::NullCheckedInvocation::invoke(onStateChanged);
        };
    chooser->launchAsync(chooserFlags, function);
}
