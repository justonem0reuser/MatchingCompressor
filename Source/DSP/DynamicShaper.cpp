#include "DynamicShaper.h"

template <typename SampleType>
DynamicShaper<SampleType>::DynamicShaper()
{
    envelopeFilter.setAttackTime(attackTime);
    envelopeFilter.setReleaseTime(releaseTime);
    envelopeFilter.setLevelCalculationType(balFilterType);
    gain[0] = 1.0;
}

template <typename SampleType>
void DynamicShaper<SampleType>::prepare(const juce::dsp::ProcessSpec& spec)
{
    jassert(spec.sampleRate > 0);
    jassert(spec.numChannels == 1 || spec.numChannels == 2);

    sampleRate = spec.sampleRate;
    channelsNumber = spec.numChannels;
    envelopeFilter.prepare(spec);
    lastEnv0 = lastEnv1 = 0.0;
    gainSmoothed.reset(sampleRate, gainSmoothingTimeMs * 0.001);
}

template <typename SampleType>
void DynamicShaper<SampleType>::reset()
{
    envelopeFilter.reset();
    lastEnv0 = lastEnv1 = 0.0;
    gainSmoothed.setCurrentAndTargetValue(gainSmoothed.getTargetValue());
}

template <typename SampleType>
void DynamicShaper<SampleType>::setGainSmoothingTime(SampleType newTimeMs)
{
    gainSmoothingTimeMs = newTimeMs;
    gainSmoothed.reset(sampleRate, newTimeMs * 0.001);
}

// envelope parameters setters

template <typename SampleType>
void DynamicShaper<SampleType>::setAttack(SampleType newAttack)
{
    attackTime = newAttack;
    envelopeFilter.setAttackTime(attackTime);
}

template <typename SampleType>
void DynamicShaper<SampleType>::setRelease(SampleType newRelease)
{
    releaseTime = newRelease;
    envelopeFilter.setReleaseTime(releaseTime);
}

template<typename SampleType>
void DynamicShaper<SampleType>::setBallisticFilterType(
    EnvCalculationType newType)
{
    if (newType == balFilterType)
        return;
    balFilterType = newType;
    envelopeFilter.setLevelCalculationType(balFilterType);
    seedEnvelopeFilter(lastEnv0, lastEnv1);
}

template<typename SampleType>
void DynamicShaper<SampleType>::setChannelAggregationType(
    ChannelAggregationType newType)
{
    if (newType == channelAggregationType)
        return;
    SampleType newEnv = aggregateLastEnv(newType);
    channelAggregationType = newType;
    seedEnvelopeFilter(newEnv, newEnv);
    lastEnv0 = lastEnv1 = newEnv;
}

template<typename SampleType>
void DynamicShaper<SampleType>::setEnvParameters(
    SampleType newAttack,
    SampleType newRelease,
    EnvCalculationType newBalFilterType,
    ChannelAggregationType newChannelAggregationType)
{
    setAttack(newAttack);
    setRelease(newRelease);
    setBallisticFilterType(newBalFilterType);
    setChannelAggregationType(newChannelAggregationType);
}

// compression parameters setters

template<typename SampleType>
void DynamicShaper<SampleType>::setGain(SampleType newGain)
{
    gainDb = newGain;
    SampleType newGainLinear = dbToGain(gainDb);
    if (newGainLinear == 0.0)
    {
        gainSmoothed.setCurrentAndTargetValue(newGainLinear);
        return;
    }
    if (gainSmoothed.getCurrentValue() == 0.0)
        gainSmoothed.setCurrentAndTargetValue(silenceGain);
    gainSmoothed.setTargetValue(newGainLinear);
}

template<typename SampleType>
void DynamicShaper<SampleType>::setKneeParameters(
    SampleType newThreshold,
    SampleType newRatio,
    SampleType newWidthDb,
    int kneeIndex)
{
    updateOneKneeParameters(newThreshold, newRatio, newWidthDb, kneeIndex);
    updateOneKneeGain(kneeIndex, true);
}

template<typename SampleType>
void DynamicShaper<SampleType>::setCompParameters(
    KneesArray& newThresholdsDb,
    KneesArray& newRatios,
    KneesArray& newWidthsDb,
    SampleType newGainDb,
    int kneesNumber)
{
    size = kneesNumber;
    setGain(newGainDb);
    for (int i = 0; i < size; i++)
    {
        updateOneKneeParameters(newThresholdsDb[i], newRatios[i], newWidthsDb[i], i);
        updateOneKneeGain(i, false);
    }
}

// processing

template<typename SampleType>
SampleType DynamicShaper<SampleType>::calculateEnv(
    int channel,
    SampleType inputValue)
{
    SampleType env = envelopeFilter.processSample(channel, inputValue);
    if (channel == 0)
        lastEnv0 = env;
    else
        lastEnv1 = env;
    return env;
}

template<typename SampleType>
void DynamicShaper<SampleType>::calculateStereoEnv(SampleType inputValue0, SampleType inputValue1, SampleType& env0, SampleType& env1)
{
    switch (channelAggregationType)
    {
    case ChannelAggregationType::separate:
    {
        env0 = envelopeFilter.processSample(0, inputValue0);
        env1 = envelopeFilter.processSample(1, inputValue1);
        break;
    }
    case ChannelAggregationType::max:
    {
        env0 = env1 = calculateStereoEnvMax(inputValue0, inputValue1);
        break;
    }
    case ChannelAggregationType::mean:
    {
        env0 = env1 = calculateStereoEnvMean(inputValue0, inputValue1);
        break;
    }
    }
    lastEnv0 = env0;
    lastEnv1 = env1;
}

template<typename SampleType>
SampleType DynamicShaper<SampleType>::calculateStereoEnvMax(
    SampleType inputValue0,
    SampleType inputValue1)
{
    SampleType maxValue = std::fmax(std::fabs(inputValue0), std::fabs(inputValue1));
    return envelopeFilter.processSample(0, maxValue);
}

template<typename SampleType>
SampleType DynamicShaper<SampleType>::calculateStereoEnvMean(
    SampleType inputValue0,
    SampleType inputValue1)
{
    SampleType meanValue =
        balFilterType == EnvCalculationType::peak ?
        (SampleType)0.5 * (std::fabs(inputValue0) + std::fabs(inputValue1)) :
        std::sqrt((SampleType)0.5 * (inputValue0 * inputValue0 + inputValue1 * inputValue1));
    return envelopeFilter.processSample(0, meanValue);
}

template<typename SampleType>
SampleType DynamicShaper<SampleType>::calculateGain(
    SampleType inputValue,
    SampleType envValue)
{
    if (size == 0)
        return inputValue;
    
    SampleType makeUpGain = gainSmoothed.getCurrentValue();

    // find knee index
    int i = -1;
    for (int j = 0; j < size; j++)
        i += (int)(envValue > kneeLeftBound[j]);

    if (i < 0)
        return makeUpGain * inputValue;

    SampleType coeff;
    if (envValue >= kneeRightBound[i])
        // (envValue * thresholdInverse[i]) ^ ratioInverseMinusOne[i]
        coeff = powCoeff[i] * std::exp2(ratioInverseMinusOne[i] * std::log2(envValue));
    else
    {
        SampleType envDb = juce::Decibels::gainToDecibels(envValue);
        SampleType envYDb = envDb * (envDb * aQuadCoeff[i] + bQuadCoeff[i]) + cQuadCoeff[i];
        coeff = dbToGain(envYDb - envDb);
    }
    return makeUpGain * gain[i] * inputValue * coeff;
}

// private methods

template<typename SampleType>
void DynamicShaper<SampleType>::seedEnvelopeFilter(
    SampleType envValue0,
    SampleType envValue1)
{
    if (channelsNumber <= 0) // not prepared yet
        return;

    envelopeFilter.setAttackTime((SampleType)0.0);
    envelopeFilter.setReleaseTime((SampleType)0.0);
    envelopeFilter.processSample(0, envValue0);
    if (channelsNumber > 1)
        envelopeFilter.processSample(1, envValue1);
    envelopeFilter.setAttackTime(attackTime);
    envelopeFilter.setReleaseTime(releaseTime);
}

template<typename SampleType>
SampleType DynamicShaper<SampleType>::aggregateLastEnv(
    ChannelAggregationType type) const
{
    switch (type)
    {
    case ChannelAggregationType::max:
        return std::fmax(lastEnv0, lastEnv1);
    case ChannelAggregationType::mean:
        return
            balFilterType == EnvCalculationType::peak ?
            (SampleType)0.5 * (lastEnv0 + lastEnv1) :
            std::sqrt((SampleType)0.5 * (lastEnv0 * lastEnv0 + lastEnv1 * lastEnv1));
    default:
        return (SampleType)0.5 * (lastEnv0 + lastEnv1);
    }
}

template<typename SampleType>
void DynamicShaper<SampleType>::updateOneKneeGain(
    int kneeIndex,
    bool updateNextGains)
{
    int iMin = std::max(kneeIndex, 1);
    int iMax = updateNextGains ? size - 1 : kneeIndex;
    for (int i = iMin; i <= iMax; i++)
        gain[i] = gain[i - 1] * std::pow(threshold[i] / threshold[i - 1], ratioInverseMinusOne[i - 1]);
}

template<typename SampleType>
void DynamicShaper<SampleType>::updateOneKneeParameters(
    SampleType newThreshold,
    SampleType newRatio,
    SampleType newWidthDb,
    int kneeIndex)
{
    jassert(
        kneeIndex >= 0 &&
        kneeIndex < size &&
        newThreshold <= 0.0 &&
        newRatio > 0.0 &&
        newWidthDb >= 0.0);

    SampleType prevRatioInverse =
        kneeIndex == 0 ?
        1.0 :
        ratioInverseMinusOne[kneeIndex - 1] + 1.0;

    SampleType newKneeWidth = newWidthDb >= minKneeWidth ? newWidthDb : 0;
    threshold[kneeIndex] = dbToGain(newThreshold);
    thresholdInverse[kneeIndex] = 1.0 / threshold[kneeIndex];
    ratioInverseMinusOne[kneeIndex] = 1.0 / newRatio - 1.0;
    powCoeff[kneeIndex] = std::pow(thresholdInverse[kneeIndex], ratioInverseMinusOne[kneeIndex]);
    kneeLeftBoundDb[kneeIndex] = newThreshold - 0.5f * newKneeWidth;
    kneeLeftBound[kneeIndex] = dbToGain(kneeLeftBoundDb[kneeIndex]);
    kneeRightBound[kneeIndex] = dbToGain(newThreshold + 0.5f * newKneeWidth);

    if (newKneeWidth > 0)
    {
        aQuadCoeff[kneeIndex] = 0.5 * (ratioInverseMinusOne[kneeIndex] + 1.0 - prevRatioInverse) / newKneeWidth;
        bQuadCoeff[kneeIndex] = prevRatioInverse - 2.0 * aQuadCoeff[kneeIndex] * kneeLeftBoundDb[kneeIndex];
        cQuadCoeff[kneeIndex] = newThreshold -
            0.5 * prevRatioInverse * newKneeWidth -
            kneeLeftBoundDb[kneeIndex] * (kneeLeftBoundDb[kneeIndex] * aQuadCoeff[kneeIndex] + bQuadCoeff[kneeIndex]);
    }
    else
        aQuadCoeff[kneeIndex] = bQuadCoeff[kneeIndex] = cQuadCoeff[kneeIndex] = 0.0;
}

template<typename SampleType>
SampleType DynamicShaper<SampleType>::dbToGain(SampleType decibels)
{
    return decibels > minusInfinityDb ? std::exp2(decibels * dbToGainCoeff) : 0.0;
}

//==============================================================================
template class DynamicShaper<float>;
template class DynamicShaper<double>;
