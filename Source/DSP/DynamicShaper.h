#pragma once
#include <JuceHeader.h>

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
    constexpr static SampleType minusInfinityDb = (SampleType)-200.0;
    const static int maxKneesNumber = 3;

    using KneesArray = std::array<SampleType, maxKneesNumber>;

    enum class ChannelAggregationType
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
            gainSmoothed.skip((int)numSamples);
            return;
        }

        const bool isListenOn = isHpfEnabled && isHpfListenOn;

        if (channelsNumber == 1)
        {
            auto* inputSamples = inputBlock.getChannelPointer(0);
            auto* outputSamples = outputBlock.getChannelPointer(0);
            SampleType env = lastEnv0;

            for (auto i = 0; i < numSamples; i++)
            {
                gainSmoothed.getNextValue();
                SampleType envInput = calculateEnvInput(0, inputSamples[i]);
                env = envelopeFilter.processSample(0, envInput);
                outputSamples[i] = isListenOn ? envInput : calculateGain(inputSamples[i], env);
            }
            lastEnv0 = lastEnv1 = env;
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
                SampleType env0 = lastEnv0, env1 = lastEnv1;
                for (auto i = 0; i < numSamples; i++)
                {
                    gainSmoothed.getNextValue();
                    SampleType envInput0 = calculateEnvInput(0, inputSamples0[i]);
                    SampleType envInput1 = calculateEnvInput(1, inputSamples1[i]);
                    env0 = envelopeFilter.processSample(0, envInput0);
                    env1 = envelopeFilter.processSample(1, envInput1);
                    outputSamples0[i] = isListenOn ? envInput0 : calculateGain(inputSamples0[i], env0);
                    outputSamples1[i] = isListenOn ? envInput1 : calculateGain(inputSamples1[i], env1);
                }
                lastEnv0 = env0;
                lastEnv1 = env1;
                break;
            }
            case ChannelAggregationType::max:
            {
                SampleType env = lastEnv0;
                for (auto i = 0; i < numSamples; i++)
                {
                    gainSmoothed.getNextValue();
                    SampleType envInput0, envInput1;
                    env = calculateStereoEnvMax(inputSamples0[i], inputSamples1[i], envInput0, envInput1);
                    outputSamples0[i] = isListenOn ? envInput0 : calculateGain(inputSamples0[i], env);
                    outputSamples1[i] = isListenOn ? envInput1 : calculateGain(inputSamples1[i], env);
                }
                lastEnv0 = lastEnv1 = env;
                break;
            }
            case ChannelAggregationType::mean:
            {
                SampleType env = lastEnv0;
                for (auto i = 0; i < numSamples; i++)
                {
                    gainSmoothed.getNextValue();
                    SampleType envInput0, envInput1;
                    env = calculateStereoEnvMean(inputSamples0[i], inputSamples1[i], envInput0, envInput1);
                    outputSamples0[i] = isListenOn ? envInput0 : calculateGain(inputSamples0[i], env);
                    outputSamples1[i] = isListenOn ? envInput1 : calculateGain(inputSamples1[i], env);
                }
                lastEnv0 = lastEnv1 = env;
                break;
            }
            }
        }
    }

    // envelope parameters setters
    void setHpfEnabled(bool newIsHpfEnabled);
    void setHpfFrequency(SampleType newHpfFrequency);
    void setAttack(SampleType newAttack);
    void setRelease(SampleType newRelease);
    void setBallisticFilterType(EnvCalculationType newType);
    void setChannelAggregationType(ChannelAggregationType newType);
    void setEnvParameters(
        bool newIsHpfEnabled,
        SampleType newHpfFrequency,
        SampleType newAttack,
        SampleType newRelease,
        EnvCalculationType newBalFilterType,
        ChannelAggregationType newChannelAggregationType);

    void setHpfListen(bool newIsHpfListenOn);

    void setGainSmoothingTime(SampleType newTimeMs);

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
    constexpr static SampleType zero = (SampleType)0.0;
    constexpr static SampleType half = (SampleType)0.5;
    constexpr static SampleType one = (SampleType)1.0;

    constexpr static SampleType dbToGainCoeff = (SampleType)0.1660964047443681;

    constexpr static SampleType silenceGain = (SampleType)1.0e-6;

    int size = 0;
    int channelsNumber = 0;
    double sampleRate = 44100.0;
    bool isHpfEnabled = false;
    bool isHpfListenOn = false;
    SampleType attackTime = 10.0, releaseTime = 100.0, gainDb = 0.0, hpfFrequency = 100.0;
    SampleType gainSmoothingTimeMs = 0.0;
    EnvCalculationType balFilterType = EnvCalculationType::peak;
    ChannelAggregationType channelAggregationType = ChannelAggregationType::separate;

    SampleType lastEnv0 = 0.0, lastEnv1 = 0.0;

    juce::SmoothedValue<SampleType, juce::ValueSmoothingTypes::Multiplicative>
        gainSmoothed{ one };

    KneesArray
        gain,
        threshold,
        thresholdInverse,
        ratioInverseMinusOne,
        powCoeff,
        kneeLeftBoundDb,
        kneeLeftBound,
        kneeRightBound,
        kneeLeftReductionDb,
        prevRatioInverseMinusOne,
        kneeQuadCoeff;

    juce::dsp::BallisticsFilter<SampleType> envelopeFilter;
    juce::dsp::StateVariableTPTFilter<SampleType> hpf;

    void updateOneKneeGain(int kneeIndex, bool updateNextGains);
    void updateOneKneeParameters(
        SampleType newThreshold,
        SampleType newRatio,
        SampleType newWidthDb,
        int kneeIndex);

    inline SampleType calculateStereoEnvMax(
        SampleType inputValue0,
        SampleType inputValue1,
        SampleType& envInput0,
        SampleType& envInput1)
    {
        envInput0 = calculateEnvInput(0, inputValue0);
        envInput1 = calculateEnvInput(1, inputValue1);
        SampleType maxValue = std::fmax(std::fabs(envInput0), std::fabs(envInput1));
        return envelopeFilter.processSample(0, maxValue);
    }

    inline SampleType calculateStereoEnvMean(
        SampleType inputValue0,
        SampleType inputValue1,
        SampleType& envInput0,
        SampleType& envInput1)
    {
        envInput0 = calculateEnvInput(0, inputValue0);
        envInput1 = calculateEnvInput(1, inputValue1);
        SampleType meanValue =
            balFilterType == EnvCalculationType::peak ?
            half * (std::fabs(envInput0) + std::fabs(envInput1)) :
            std::sqrt(half * (envInput0 * envInput0 + envInput1 * envInput1));
        return envelopeFilter.processSample(0, meanValue);
    }

    inline SampleType calculateEnvInput(int channel, SampleType inputValue)
    {
        SampleType filtered = hpf.processSample(channel, inputValue);
        return isHpfEnabled ? filtered : inputValue;
    }

    void seedEnvelopeFilter(SampleType envValue0, SampleType envValue1);
    SampleType aggregateLastEnv(ChannelAggregationType type) const;

    // quicker version of juce::Decibels::decibelsToGain
    static inline SampleType dbToGain(SampleType decibels);
};
