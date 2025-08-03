#pragma once
#include <JuceHeader.h>
#include "alglibinternal.h"

using EnvCalculationType = juce::dsp::BallisticsFilterLevelCalculationType;

/// <summary>
/// The core audio processing class - 
/// up-to-three knees compressor/expander
/// </summary>
/// <typeparam name="SampleType"></typeparam>
template <typename SampleType>
class DynamicShaper
{
public:
    constexpr static SampleType minusInfinityDb = -200.0;
    constexpr static SampleType minKneeWidth = 0.1;
    const static int maxKneesNumber = 3;

    using KneesArray = std::array<SampleType, maxKneesNumber>;

    enum ChannelAggregationType
    {
        separate,
        max,
        mean,
    };

    DynamicShaper();

    /// <summary>
    /// Audio block processing preparation
    /// (called from AudioProcessor prepareToPlay method).
    /// </summary>
    virtual void prepare(const juce::dsp::ProcessSpec& spec);
    
    void reset();

    /// <summary>
    /// Audio block processing
    /// (called from AudioProcessor processBlock method).
    /// </summary>
    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        const auto numSamples = outputBlock.getNumSamples();

        jassert(inputBlock.getNumSamples() == numSamples);

        if (context.isBypassed)
        {
            outputBlock.copyFrom(inputBlock);
            return;
        }

        if (channelsNumber == 1)
        {
            auto* inputSamples = inputBlock.getChannelPointer(0);
            auto* outputSamples = outputBlock.getChannelPointer(0);

            for (auto i = 0; i < numSamples; i++)
                outputSamples[i] = processSample(0, inputSamples[i]);
        }
        else
        {
            auto* inputSamples0 = inputBlock.getChannelPointer(0);
            auto* inputSamples1 = inputBlock.getChannelPointer(1);
            auto* outputSamples0 = outputBlock.getChannelPointer(0);
            auto* outputSamples1 = outputBlock.getChannelPointer(1);

            for (auto i = 0; i < numSamples; i++)
            {
                auto samplePair = processStereoSample(inputSamples0[i], inputSamples1[i]);
                outputSamples0[i] = samplePair.first;
                outputSamples1[i] = samplePair.second;
            }
        }
    }

    // envelope parameters setters
    void setAttack(SampleType newAttack);
    void setRelease(SampleType newRelease);
    void setBallisticFilterType(EnvCalculationType newType);
    void setChannelAggregationType(ChannelAggregationType newType);
    void setEnvParameters(
        SampleType newAttack,
        SampleType newRelease,
        EnvCalculationType newBalFilterType,
        ChannelAggregationType newChannelAggregationType);

    // compression parameters setters
    void setGain(SampleType newGain);
    void setKneeParameters(
        SampleType newThreshold,
        SampleType newRatio,
        SampleType newWidthDb,
        int kneeIndex);
    void setCompParameters(
        KneesArray& newThresholds,
        KneesArray& newRatios,
        KneesArray& newWidthsDb,
        SampleType newGain,
        int kneesNumber);

    // processing
    SampleType processSample(int channel, SampleType inputValue);
    std::pair<SampleType, SampleType> processStereoSample(SampleType inputValue0, SampleType inputValue1);
    SampleType calculateEnv(int channel, SampleType inputValue);
    std::pair<SampleType, SampleType> calculateStereoEnv(SampleType inputValue0, SampleType inputValue1);
    SampleType calculateGain(SampleType inputValue, SampleType envValue);

private:
    int size = 0;
    int channelsNumber = 0;
    double sampleRate = 44100.0;
    SampleType attackTime = 1.0, releaseTime = 100.0, gainDb = 0.0;
    EnvCalculationType balFilterType = EnvCalculationType::peak;
    ChannelAggregationType channelAggregationType = ChannelAggregationType::separate;
    
    KneesArray
        gain, 
        threshold, 
        thresholdInverse, 
        ratioInverseMinusOne,
        kneeLeftBoundDb,
        kneeLeftBound,
        kneeRightBound,
        aQuadCoeff,
        bQuadCoeff,
        cQuadCoeff;

    juce::dsp::BallisticsFilter<SampleType> envelopeFilter;

    int findKneeIndex(SampleType value);
    void updateOneKneeGain(int kneeIndex, bool updateNextGains);
    void updateOneKneeParameters(
        SampleType newThreshold,
        SampleType newRatio,
        SampleType newWidthDb,
        int kneeIndex);
};

