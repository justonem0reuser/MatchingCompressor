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
            SampleType env;

            for (auto i = 0; i < numSamples; i++)
            {
                env = envelopeFilter.processSample(0, inputSamples[i]);
                outputSamples[i] = calculateGain(inputSamples[i], env);
            }
        }
        else
        {
            auto* inputSamples0 = inputBlock.getChannelPointer(0);
            auto* inputSamples1 = inputBlock.getChannelPointer(1);
            auto* outputSamples0 = outputBlock.getChannelPointer(0);
            auto* outputSamples1 = outputBlock.getChannelPointer(1);

            switch (channelAggregationType)
            {
            case ChannelAggregationType::separate:
            {
                SampleType env0, env1;
                for (auto i = 0; i < numSamples; i++)
                {
                    env0 = envelopeFilter.processSample(0, inputSamples0[i]);
                    env1 = envelopeFilter.processSample(1, inputSamples1[i]);
                    outputSamples0[i] = calculateGain(inputSamples0[i], env0);
                    outputSamples1[i] = calculateGain(inputSamples1[i], env1);
                }
                break;
            }
            case ChannelAggregationType::max:
            {
                SampleType env;
                for (auto i = 0; i < numSamples; i++)
                {
                    env = calculateStereoEnvMax(inputSamples0[i], inputSamples1[i]);
                    outputSamples0[i] = calculateGain(inputSamples0[i], env);
                    outputSamples1[i] = calculateGain(inputSamples1[i], env);
                }
                break;
            }
            case ChannelAggregationType::mean:
            {
                SampleType env;
                for (auto i = 0; i < numSamples; i++)
                {
                    env = calculateStereoEnvMean(inputSamples0[i], inputSamples1[i]);
                    outputSamples0[i] = calculateGain(inputSamples0[i], env);
                    outputSamples1[i] = calculateGain(inputSamples1[i], env);
                }
                break;
            }
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
    inline SampleType calculateGain(SampleType inputValue, SampleType envValue);
    
    // for non-realtime calls
    SampleType calculateEnv(int channel, SampleType inputValue);
    void calculateStereoEnv(SampleType inputValue0, SampleType inputValue1, SampleType& env0, SampleType& env1);

private:
    constexpr static SampleType dbToGainCoeff = 0.1660964047443681;

    int size = 0;
    int channelsNumber = 0;
    double sampleRate = 44100.0;
    SampleType attackTime = 10.0, releaseTime = 100.0, gainDb = 0.0;
    EnvCalculationType balFilterType = EnvCalculationType::peak;
    ChannelAggregationType channelAggregationType = ChannelAggregationType::separate;
    
    KneesArray
        gain, 
        threshold, 
        thresholdInverse, 
        ratioInverseMinusOne,
        powCoeff,
        kneeLeftBoundDb,
        kneeLeftBound,
        kneeRightBound,
        aQuadCoeff,
        bQuadCoeff,
        cQuadCoeff;

    juce::dsp::BallisticsFilter<SampleType> envelopeFilter;

    void updateOneKneeGain(int kneeIndex, bool updateNextGains);
    void updateOneKneeParameters(
        SampleType newThreshold,
        SampleType newRatio,
        SampleType newWidthDb,
        int kneeIndex);

    inline SampleType calculateStereoEnvMax(SampleType inputValue0, SampleType inputValue1);
    inline SampleType calculateStereoEnvMean(SampleType inputValue0, SampleType inputValue1);

    // quicker version of juce::Decibels::decibelsToGain
    static inline SampleType dbToGain(SampleType decibels);
};
