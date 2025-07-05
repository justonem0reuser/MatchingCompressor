#include "DataReceiverController.h"
#include "../Data/Messages.h"

DataReceiverController::DataReceiverController(
	BaseDataReceiver* dataReceiver,
	MatchingData& matchingData,
    MatchCompressorAudioProcessor& processor) :
	matchingData(matchingData),
    dataReceiver(dataReceiver),
    processor(processor)
{
    this->dataReceiver->onFileChosen = [this](juce::File& file, bool isRef) 
        { 
            getFromFile(file, isRef); 
        };
    this->dataReceiver->onRefReceivedFromBus = [this] 
        { 
            checkAndSaveData(this->matchingData.newRefSamples, this->matchingData.newRefSampleRate, false, true); 
        };
    this->dataReceiver->onDestReceivedFromBus = [this] 
        { 
            checkAndSaveData(this->matchingData.newDestSamples, this->matchingData.newDestSampleRate, false, false); 
        };
    this->dataReceiver->onCollectFromBusStateChanged = [this](bool mainBus, bool sidechain)
        {
            this->processor.setDataCollectionBuses(mainBus, sidechain);
        };
}

void DataReceiverController::setFromDataCollector(
    std::vector<std::vector<float>>& refSamples, 
    double refSampleRate, 
    std::vector<std::vector<float>>& destSamples, 
    double destSampleRate)
{
    if (dataReceiver->isReadRefFromStreamEnabled())
    {
        matchingData.newRefSamples = refSamples;
        matchingData.newRefSampleRate = refSampleRate;
        dataReceiver->setNeedChangeRefLabelText();
    }
    if (dataReceiver->isReadDestFromStreamEnabled())
    {
        matchingData.newDestSamples = destSamples;
        matchingData.newDestSampleRate = destSampleRate;
        dataReceiver->setNeedChangeDestLabelText();
    }
}

void DataReceiverController::getReceivedData(
    std::vector<std::vector<float>>& refSamples, 
    double& refSampleRate, 
    std::vector<std::vector<float>>& destSamples, 
    double& destSampleRate) const
{
    refSamples = matchingData.refSamples;
    refSampleRate = matchingData.refSampleRate;
    destSamples = matchingData.destSamples;
    destSampleRate = matchingData.destSampleRate;
}

void DataReceiverController::getFromFile(juce::File& file, bool isRef)
{
	std::vector<std::vector<float>> newSamples;
	double newSampleRate;
	audioFileReader.readFromFile(file, true, newSamples, newSampleRate);
    checkAndSaveData(newSamples, newSampleRate, true, isRef);
    if (isRef)
        dataReceiver->setRefDataState(SetDataState::SetFromFile);
    else
        dataReceiver->setDestDataState(SetDataState::SetFromFile);
}

void DataReceiverController::checkAndSaveData(
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

        if (isRef)
        {
            matchingData.refSamples = samples;
            matchingData.refSampleRate = sampleRate;
        }
        else
        {
            matchingData.destSamples = samples;
            matchingData.destSampleRate = sampleRate;
        }
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
