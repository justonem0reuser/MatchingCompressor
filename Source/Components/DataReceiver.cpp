#include "DataReceiver.h"
#include "../Messages.h"
#include "../ExceptionHelper.h"
#include "../ComponentInitializerHelper.h"

DataReceiver::DataReceiver(
    bool isMainBusConnected, bool 
    isSidechainConnected, 
    std::function<void(bool, bool)> toggleFunc):
    refEnabled(isSidechainConnected),
    destEnabled(isMainBusConnected),
    refToggleButtonText(isSidechainConnected ? readFromSidechainBusStr : sidechainBusIsDisconnectedStr),
    destToggleButtonText(isMainBusConnected ? readFromMainBusStr : mainBusIsDisconnectedStr),
    mToggleFuncBool(toggleFunc)

{
    formatManager.registerBasicFormats();
    auto refToggleFunc = [&] {
        refChecked = readRefFromStreamButton.getToggleState();
        mToggleFuncBool(
            readDestFromStreamButton.getToggleState(),
            readRefFromStreamButton.getToggleState());
        };
    auto destToggleFunc = [&] {
        destChecked = readDestFromStreamButton.getToggleState();
        mToggleFuncBool(
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

bool DataReceiver::isAllDataSet() const
{
    return isRefDataSet && isDestDataSet;
}

void DataReceiver::getReceivedData(
    std::vector<std::vector<float>>& refSamples,
    double& refSampleRate,
    std::vector<std::vector<float>>& destSamples,
    double& destSampleRate) const
{
    refSamples = this->refSamples;
    refSampleRate = this->refSampleRate;
    destSamples = this->destSamples;
    destSampleRate = this->destSampleRate;
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

void DataReceiver::setFromDataCollector(
    std::vector<std::vector<float>>& refSamples,
    double refSampleRate,
    std::vector<std::vector<float>>& destSamples,
    double destSampleRate)
{
    if (readRefFromStreamButton.getToggleState())
    {
        newRefSamples = refSamples;
        newRefSampleRate = refSampleRate;
        needChangeRefLabelText = true;
    }
    if (readDestFromStreamButton.getToggleState())
    {
        newDestSamples = destSamples;
        newDestSampleRate = destSampleRate;
        needChangeDestLabelText = true;
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
            checkSourceDestFileData(newRefSamples, newRefSampleRate, false, true);
            refSamples = newRefSamples;
            refSampleRate = newRefSampleRate;
            isRefDataSet = true;
            refIsSetLabel.setText(refSetFromBusStr, juce::NotificationType::dontSendNotification);
            refIsSetLabel.repaint();
            juce::NullCheckedInvocation::invoke(onStateChanged);

        }
        if (needChangeDestLabelText)
        {
            checkSourceDestFileData(newDestSamples, newDestSampleRate, false, false);
            destSamples = newDestSamples;
            destSampleRate = newDestSampleRate;
            isDestDataSet = true;
            destIsSetLabel.setText(destSetFromBusStr, juce::NotificationType::dontSendNotification);
            destIsSetLabel.repaint();
            juce::NullCheckedInvocation::invoke(onStateChanged);
        }
    }
    catch (const std::exception& e)
    {
        ExceptionHelper::catchException(e, this);
    }
    needChangeRefLabelText = false;
    needChangeDestLabelText = false;

    juce::NullCheckedInvocation::invoke(onTimer);

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
                {
                    std::vector<std::vector<float>> newSamples;
                    double newSampleRate;
                    readFromFile(file, true, newSamples, newSampleRate);
                    checkSourceDestFileData(newSamples, newSampleRate, true, isRef);
                    if (isRef)
                    {
                        refSamples = newSamples;
                        refSampleRate = newSampleRate;
                        isRefDataSet = true;
                        refIsSetLabel.setText(refSetFromFileStr, juce::NotificationType::dontSendNotification);
                        refIsSetLabel.repaint();
                    }
                    else
                    {
                        destSamples = newSamples;
                        destSampleRate = newSampleRate;
                        isDestDataSet = true;
                        destIsSetLabel.setText(destSetFromFileStr, juce::NotificationType::dontSendNotification);
                        destIsSetLabel.repaint();
                    }
                }
            }
            catch (const std::exception& e)
            {
                ExceptionHelper::catchException(e, this);
            }
            juce::NullCheckedInvocation::invoke(onStateChanged);
        };
    chooser->launchAsync(chooserFlags, function);
}

void DataReceiver::readFromFile(
    juce::File file,
    bool excludeZeroSamples,
    std::vector<std::vector<float>>& res,
    double& sampleRate)
{
    sampleRate = 0.0;

    if (file != juce::File{})
    {
        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(file));
        if (reader != nullptr)
        {
            auto length = reader->lengthInSamples;
            auto numChannels = reader->numChannels;
            sampleRate = reader->sampleRate;
            if (length > 0 && numChannels > 0 && sampleRate > 0)
            {
                juce::AudioBuffer<float> buffer(numChannels, length);
                buffer.clear();
                if (reader->read(buffer.getArrayOfWritePointers(), numChannels, 0, length))
                {
                    res.clear();
                    for (int i = 0; i < numChannels; i++)
                    {
                        res.push_back(std::vector<float>{});
                        res[i].reserve(length);
                    }
                    if (excludeZeroSamples)
                        for (int i = 0; i < length; i++)
                        { 
                            bool isZero = true;
                            for (int j = 0; j < numChannels; j++)
                                isZero &= buffer.getSample(j, i) == 0;
                            if (!isZero)
                                for (int j = 0; j < numChannels; j++)
                                    res[j].push_back(buffer.getSample(j, i));
                        }
                    else
                        for (int j = 0; j < numChannels; j++)
                        {
                            auto* readPointer = buffer.getReadPointer(j);
                            res[j].insert(res[j].begin(), readPointer, readPointer + length);
                        }

                }
            }
        }
    }
}

void DataReceiver::checkSourceDestFileData(
    std::vector<std::vector<float>> samples,
    double sampleRate,
    bool isFile,
    bool isRef)
{
    try
    {
        if (sampleRate <= 0)
            throw std::exception(sampleRateIsNullExStr.getCharPointer());
        if (samples.size() <= 0 || samples.size() > 2)
            throw std::exception(numChannelsIsNullExStr.getCharPointer());
        if (samples[0].size() <= 0)
            throw std::exception(lengthIsNullExStr.getCharPointer());
        if (samples.size() > 1)
            for (int i = 1; i < samples.size(); i++)
                if (samples[i].size() != samples[0].size())
                    throw std::exception(corruptedChannelExStr.getCharPointer());
    }
    catch (const std::exception& e)
    {
        auto res(
            isFile ? (isRef ? refFileExStr : destFileExStr) :
            (isRef ? refStreamExStr : destStreamExStr));
        int i = res.length();
        res.append(e.what(), 500);
        throw std::exception(res.getCharPointer());
    }
}