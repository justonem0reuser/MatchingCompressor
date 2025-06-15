#pragma once
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Messages.h"
#include "Ranges.h"

using ChannelAggregationType = DynamicShaper<float>::ChannelAggregationType;

//==============================================================================
MatchCompressorAudioProcessor::MatchCompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
         .withInput("Input", juce::AudioChannelSet::stereo(), true)
         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
         .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
     )
#endif
{
    chain.get<ChainPositions::MainBusCollector>().setStartChannelNumber(0);
    chain.get<ChainPositions::SidechainCollector>().setStartChannelNumber(0);
    chain.setBypassed<ChainPositions::MainBusCollector>(true);
    chain.setBypassed<ChainPositions::SidechainCollector>(true);
    chain.setBypassed<ChainPositions::CompressorExpander>(false);
}

//==============================================================================
void MatchCompressorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mainBusNumChannels = getMainBusNumInputChannels();
    sidechainNumChannels = getNumInputChannels() - mainBusNumChannels;

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = mainBusNumChannels;
    chain.get<ChainPositions::MainBusCollector>().prepare(spec);
    chain.get<ChainPositions::CompressorExpander>().prepare(spec);

    spec.numChannels = sidechainNumChannels;
    auto& sidechainCollector = chain.get<ChainPositions::SidechainCollector>();
    sidechainCollector.prepare(spec);
    sidechainCollector.setStartChannelNumber(mainBusNumChannels);

    juce::NullCheckedInvocation::invoke(onPrepareToPlay);

    updateCompressorParameters();
}

void MatchCompressorAudioProcessor::releaseResources()
{
    // This method is called when playback stops
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MatchCompressorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif
    if (layouts.getChannelSet(false, 1) != juce::AudioChannelSet::disabled()
        && layouts.getChannelSet(false, 1) != juce::AudioChannelSet::mono()
        && layouts.getChannelSet(false, 1) != juce::AudioChannelSet::stereo())
        return false;
    return true;
#endif
}
#endif

void MatchCompressorAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    if (needUpdate)
        updateCompressorParameters();
    bool currentIsPlayHeadPlaying = isPlayHeadPlaying();
    if (currentIsPlayHeadPlaying)
    {
        if (!prevIsPlayHeadPlaying)
        {
            if (collectMainBusData)
                chain.get<ChainPositions::MainBusCollector>().reset();
            if (collectSidechainData)
                chain.get<ChainPositions::SidechainCollector>().reset();
            chain.setBypassed<ChainPositions::MainBusCollector>(!collectMainBusData);
            chain.setBypassed<ChainPositions::SidechainCollector>(!collectSidechainData);
            juce::NullCheckedInvocation::invoke(onPlayHeadStartPlaying);
        }
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        chain.process(context);
    }
    else if (prevIsPlayHeadPlaying)
    {
        chain.setBypassed<ChainPositions::MainBusCollector>(true);
        chain.setBypassed<ChainPositions::SidechainCollector>(true);
        chain.get<ChainPositions::CompressorExpander>().reset();
        juce::NullCheckedInvocation::invoke(onPlayHeadStopPlaying);
    }
    prevIsPlayHeadPlaying = currentIsPlayHeadPlaying;
}

//==============================================================================
juce::AudioProcessorEditor* MatchCompressorAudioProcessor::createEditor()
{
    return new MatchCompressorAudioProcessorEditor(*this);
}

bool MatchCompressorAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String MatchCompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MatchCompressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MatchCompressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MatchCompressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MatchCompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================
int MatchCompressorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MatchCompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MatchCompressorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MatchCompressorAudioProcessor::getProgramName (int index)
{
    return {};
}

void MatchCompressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MatchCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void MatchCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
        apvts.replaceState(tree);
}

// Data collecting

bool MatchCompressorAudioProcessor::isInputBusConnected(int i)
{
    auto bus = getBus(true, i);
    if (!bus)
        return false;
    return bus->isEnabled() && bus->getNumberOfChannels() > 0;
}

bool MatchCompressorAudioProcessor::isPlayHeadPlaying() const
{
    if (auto playHead = getPlayHead()) // some hosts do not provide this
    {
        juce::AudioPlayHead::CurrentPositionInfo positionInfo;
        if (playHead->getCurrentPosition(positionInfo))
            return positionInfo.isPlaying;
    }
    return false; // Assume not playing if no playhead or info available
}

void MatchCompressorAudioProcessor::setDataCollectionBuses(bool mainBus, bool sidechain)
{
    collectMainBusData = mainBus;
    collectSidechainData = sidechain;
}

void MatchCompressorAudioProcessor::setMemoryFullFunc(std::function<void()> func)
{
    chain.get<ChainPositions::MainBusCollector>().onMemoryFull = func;
    chain.get<ChainPositions::SidechainCollector>().onMemoryFull = func;
}

void MatchCompressorAudioProcessor::getCollectedData(
    std::vector<std::vector<float>>& mainBusData, 
    double& mainBusRate,
    std::vector<std::vector<float>>& sidechainData, 
    double& sidechainRate)
{
    chain.setBypassed<ChainPositions::MainBusCollector>(true);
    chain.setBypassed<ChainPositions::SidechainCollector>(true);
    chain.get<ChainPositions::MainBusCollector>().getData(mainBusData, mainBusRate);
    chain.get<ChainPositions::SidechainCollector>().getData(sidechainData, sidechainRate);
}

// Compressor parameters setting

void MatchCompressorAudioProcessor::setNeedUpdate()
{
    needUpdate = true;
}

void MatchCompressorAudioProcessor::setCompressorParameters(
    KneesArray& thresholdDbs,
    KneesArray& ratios,
    KneesArray& widthDbs,
    float gainDb,
    int kneesNumber)
{ 
    this->gainDb = gainDb;
    this->thresholdDbs = thresholdDbs;
    this->ratios = ratios;
    this->widthDbs = widthDbs;
    this->kneesNumber = kneesNumber;
    needUpdate = true;
}

void MatchCompressorAudioProcessor::updateCompressorParameters()
{
    auto& freeShaper = chain.get<ChainPositions::CompressorExpander>();

    // reading compression/expanding parameters
    float newGainDb = *apvts.getRawParameterValue(gainId);
    bool isGainChanged = newGainDb != gainDb;
    gainDb = newGainDb;

    int newKneesNumber = *apvts.getRawParameterValue(kneesNumberId);
    bool isKneesNumberChanged = newKneesNumber != kneesNumber;
    kneesNumber = newKneesNumber;

    int changedKneesNumber = 0;
    int changedKneeIndex = 0; // will be used only if changedKneesNumber == 1
    for (int i = 0; i < kneesNumber; i++)
    {
        auto iStr = std::to_string(i);
        float newThreshold = *apvts.getRawParameterValue(thresholdId + iStr);
        float newRatio = *apvts.getRawParameterValue(ratioId + iStr);
        if (newRatio < 1.f)
            newRatio = 1.f / (2.f - newRatio);
        float newKneeWidth = *apvts.getRawParameterValue(kneeWidthId + iStr);
        
        bool isKneeChanged =
            newThreshold != thresholdDbs[i] ||
            newRatio != ratios[i] ||
            newKneeWidth != widthDbs[i];
        int isKneeChangedInt = (int)isKneeChanged;
        changedKneesNumber += isKneeChangedInt;
        changedKneeIndex += isKneeChangedInt * i;
        
        thresholdDbs[i] = newThreshold;
        ratios[i] = newRatio;
        widthDbs[i] = newKneeWidth;
    }
    
    bool isFullUpdateNeeded =
        isKneesNumberChanged || changedKneesNumber > 1 ||
        (isGainChanged && changedKneesNumber != 0);

    // trying to avoid full update if it is not necessary
    if (isFullUpdateNeeded)
        freeShaper.setCompParameters(
            thresholdDbs, 
            ratios, 
            widthDbs, 
            gainDb, 
            kneesNumber);
    else if (isGainChanged)
        freeShaper.setGain(gainDb);
    else if (changedKneesNumber != 0)
        freeShaper.setKneeParameters(
            thresholdDbs[changedKneeIndex],
            ratios[changedKneeIndex],
            widthDbs[changedKneeIndex],
            changedKneeIndex);

    // envelope parameters
    float attack = *apvts.getRawParameterValue(attackId);
    float release = *apvts.getRawParameterValue(releaseId);
    auto balFilterType =
        *apvts.getRawParameterValue(balFilterTypeId) < 1.5f ?
        EnvCalculationType::peak :
        EnvCalculationType::RMS;
    auto channelAggregationType =
        *apvts.getRawParameterValue(channelAggrerationTypeId) < 1.5f ?
        ChannelAggregationType::separate :
        *apvts.getRawParameterValue(channelAggrerationTypeId) < 2.5f ?
        ChannelAggregationType::max :
        ChannelAggregationType::mean;

    freeShaper.setEnvParameters(
        attack,
        release,
        balFilterType,
        channelAggregationType);

    needUpdate = false;
}

//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout MatchCompressorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterInt>(
        kneesNumberId, "Knees number", 1, DynamicShaper<float>::maxKneesNumber, 1));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        balFilterTypeId, "Envelope type", 1, 2, 1));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        channelAggrerationTypeId, "Stereo processing", 1, 3, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        attackId, "Attack", attackRange, 0.1f, "ms"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        releaseId, "Release", releaseRange, 100.f, "ms"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        gainId, "Gain", gainRange, 0.f, "dB",
        juce::AudioProcessorParameter::Category::genericParameter,
        dbStringFromValue, dbValueFromString));
    for (int i = 0; i < DynamicShaper<float>::maxKneesNumber; i++)
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            thresholdId + std::to_string(i), 
            "Threshold", thresholdRange, 0.f, "dB",
            juce::AudioProcessorParameter::Category::genericParameter,
            dbStringFromValue, dbValueFromString));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            ratioId + std::to_string(i),
            "Ratio", ratioRange, 1.f, "",
            juce::AudioProcessorParameter::Category::genericParameter,
            ratioStringFromValue, ratioValueFromString));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            kneeWidthId + std::to_string(i),
            "Knee Width", kneeWidthRange, 0.f, "dB",
            juce::AudioProcessorParameter::Category::genericParameter,
            dbStringFromValue, dbValueFromString));
    }
    return layout;
}

// parameters values displaying and reading

juce::String MatchCompressorAudioProcessor::ratioStringFromValue(float value, int maximumStringLength)
{
    if (value >= 1.f)
    {
        juce::String res(value, 2);
        return res + ": 1";
    }
    else
    {
        juce::String res(2.f - value, 2);
        return "1 : " + res;
    }
}
float MatchCompressorAudioProcessor::ratioValueFromString(const juce::String& text)
{
    juce::String s = text.removeCharacters(" ").replaceCharacter(',', '.');
    float res;
    if (s.startsWith("1:"))
    {
        res = s.substring(2).getFloatValue();
        if (res <= 0.f)
            res = 1.f;
        else if (res < 1.f)
        {
            res = 1.f / res;
            res = std::clamp(res, 1.f, ratioRange.end);
        }
        else
        {
            res = 2.f - res;
            res = std::clamp(res, ratioRange.start, 1.f);
        }
    }
    else
    {
        if (s.endsWith(":1"))
            s = s.substring(0, s.length() - 2);
        res = s.getFloatValue();
        if (res < 1.f && res > 0.f)
        {
            res = 2.f - 1.f / res;
            res = std::clamp(res, ratioRange.start, 1.f);
        }
        else
            res = std::clamp(res, 1.f, ratioRange.end);
    }
     
    return res;
}

juce::String MatchCompressorAudioProcessor::dbStringFromValue(float value, int maximumStringLength)
{
    return juce::String(value, 1);
}

float MatchCompressorAudioProcessor::dbValueFromString(const juce::String& text)
{
    return text.getFloatValue();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MatchCompressorAudioProcessor();
}
