#pragma once

#include <JuceHeader.h>
#include "DSP/DynamicShaper.h"
#include "DSP/DataCollector.h"

//==============================================================================
using Chain = juce::dsp::ProcessorChain<
    DataCollector<float>,
    DataCollector<float>,
    DynamicShaper<float>>;
using KneesArray = DynamicShaper<float>::KneesArray;

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

    std::function<void()> onPrepareToPlay;
    std::function<void()> onPlayHeadStartPlaying;
    std::function<void()> onPlayHeadStopPlaying;

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
    void setMemoryFullFunc(std::function<void()> func);
    void getCollectedData(
        std::vector<std::vector<float>>& mainBusData,
        double& mainBusRate,
        std::vector<std::vector<float>>& sidechainData,
        double& sidechainRate);

    // Compressor parameters setting
    void setNeedUpdate();
    void setCompressorParameters(
        KneesArray& thresholdDbs,
        KneesArray& ratios,
        KneesArray& widthDbs,
        float gainDb,
        int kneesNumber);
    void updateCompressorParameters();

private:
    Chain chain;
    
    // for data collecting
    bool prevIsPlayHeadPlaying = false;
    bool collectMainBusData = false;
    bool collectSidechainData = false;
    int mainBusNumChannels, sidechainNumChannels;

    // for compressor parameters setting
    bool needUpdate = false;
    KneesArray thresholdDbs, ratios, widthDbs;
    float gainDb;
    int kneesNumber;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // parameters values displaying and reading
    static juce::String ratioStringFromValue(float value, int maximumStringLength);
    static float ratioValueFromString(const juce::String& text);
    static juce::String dbStringFromValue(float value, int maximumStringLength);
    static float dbValueFromString(const juce::String& text);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatchCompressorAudioProcessor)
};
