#pragma once

#include <JuceHeader.h>
#include "DSP/DynamicShaper.h"
#include "DSP/DataCollector.h"
#include "Data/MatchingData.h"

//==============================================================================
using Chain = juce::dsp::ProcessorChain<
    DataCollector<float>,
    DataCollector<float>,
    DynamicShaper<float>>;
using KneesArray = DynamicShaper<float>::KneesArray;
using ParamsArray = std::array<std::atomic<float>*, DynamicShaper<float>::maxKneesNumber>;
using EnvCalculationType = juce::dsp::BallisticsFilterLevelCalculationType;
using ChannelAggregationType = DynamicShaper<float>::ChannelAggregationType;

enum ChainPositions
{
    MainBusCollector,
    SidechainCollector,
    CompressorExpander,
};

class MatchCompressorAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

    std::function<void()> PrepareToPlay;
    std::function<void()> PlayHeadStartPlaying;
    std::function<void()> PlayHeadStopPlaying;
    std::function<void()> DataCollectorMemoryFull;

    MatchCompressorAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    // TODO: add programs
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Data collecting
    bool isInputBusConnected(int i);
    bool isPlayHeadPlaying() const;
    void setDataCollectionBuses(bool mainBus, bool sidechain);
    void getCollectedData(
        std::vector<std::vector<float>>& mainBusData,
        double& mainBusRate,
        std::vector<std::vector<float>>& sidechainData,
        double& sidechainRate);

    // Compressor parameters setting
    void updateCompressorParameters();

    MatchingData& getMatchingData();

private:
    const int stateVersion = 2;

    Chain chain;
    
    // for data collecting
    bool prevIsPlayHeadPlaying = false;
    bool collectMainBusData = false;
    bool collectSidechainData = false;
    int mainBusNumChannels, sidechainNumChannels;

    // Pointers to APVTS parameters
    std::atomic<float>* gainParam;
    std::atomic<float>* kneesNumberParam;
    std::atomic<float>* attackParam;
    std::atomic<float>* releaseParam;
    std::atomic<float>* balFilterTypeParam;
    std::atomic<float>* channelAggrerationTypeParam;
    ParamsArray thresholdParams;
    ParamsArray ratioParams;
    ParamsArray kneeWidthParams;

    // for checking and setting compressor parameters
    float gainDb = -1000.f;
    int kneesNumber = -1;
    KneesArray thresholdDbs, ratios, widthDbs; // initial values are not necessary because of the initial kneesNumber 

    // for checking compressor parameters
    float attackMs = -1.f, releaseMs = -1.f;
    EnvCalculationType balFilterType = EnvCalculationType::peak;
    ChannelAggregationType channelAggregationType = ChannelAggregationType::separate;

    MatchingData matchingData;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // parameters values displaying and reading
    static juce::String ratioStringFromValue(float value, int maximumStringLength);
    static float ratioValueFromString(const juce::String& text);
    static juce::String dbStringFromValue(float value, int maximumStringLength);
    static float dbValueFromString(const juce::String& text);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatchCompressorAudioProcessor)
};
