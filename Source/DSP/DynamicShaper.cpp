#include "DynamicShaper.h"

template <typename SampleType>
DynamicShaper<SampleType>::DynamicShaper()
{
    envelopeFilter.setAttackTime(attackTime);
    envelopeFilter.setReleaseTime(releaseTime);
    envelopeFilter.setLevelCalculationType(balFilterType);
    gain[0] = juce::Decibels::decibelsToGain(gainDb, minusInfinityDb);
}

template <typename SampleType>
void DynamicShaper<SampleType>::prepare(const juce::dsp::ProcessSpec& spec)
{
    jassert(spec.sampleRate > 0);
    jassert(spec.numChannels == 1 || spec.numChannels == 2);

    sampleRate = spec.sampleRate;
    channelsNumber = spec.numChannels;
    envelopeFilter.prepare(spec);
}

template <typename SampleType>
void DynamicShaper<SampleType>::reset()
{
    envelopeFilter.reset();
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
    if (balFilterType == newType)
        return;
    balFilterType = newType;
    envelopeFilter.setLevelCalculationType(balFilterType);
}

template<typename SampleType>
void DynamicShaper<SampleType>::setChannelAggregationType(
    ChannelAggregationType newType)
{
    if (channelAggregationType == newType)
        return;
    channelAggregationType = newType;
    envelopeFilter.reset();
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
    gain[0] = juce::Decibels::decibelsToGain(gainDb, minusInfinityDb);
    updateOneKneeGain(0, true);
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
    gain[0] = juce::Decibels::decibelsToGain(newGainDb);
    for (int i = 0; i < size; i++)
    {
        updateOneKneeParameters(newThresholdsDb[i], newRatios[i], newWidthsDb[i], i);
        updateOneKneeGain(i, false);
    }
}

// processing

template <typename SampleType>
SampleType DynamicShaper<SampleType>::processSample(
    int channel, 
    SampleType inputValue)
{
    auto env = envelopeFilter.processSample(channel, inputValue);
    return calculateGain(inputValue, env);
}

template<typename SampleType>
std::pair<SampleType, SampleType> DynamicShaper<SampleType>::processStereoSample(
    SampleType inputValue0, 
    SampleType inputValue1)
{
    auto env = calculateStereoEnv(inputValue0, inputValue1);
    auto y0 = calculateGain(inputValue0, env.first);
    auto y1 = calculateGain(inputValue1, env.second);
    return std::pair<SampleType, SampleType> {y0, y1};
}

template<typename SampleType>
SampleType DynamicShaper<SampleType>::calculateEnv(
    int channel, 
    SampleType inputValue)
{
    return envelopeFilter.processSample(channel, inputValue);
}

template<typename SampleType>
std::pair<SampleType, SampleType> DynamicShaper<SampleType>::calculateStereoEnv(
    SampleType inputValue0, 
    SampleType inputValue1)
{
    switch (channelAggregationType)
    {
    case ChannelAggregationType::separate:
    {
        auto env0 = envelopeFilter.processSample(0, inputValue0);
        auto env1 = envelopeFilter.processSample(1, inputValue1);
        return std::pair<SampleType, SampleType> {env0, env1};
    }
    case ChannelAggregationType::max:
    {
        auto maxValue = std::fmax(std::fabsf(inputValue0), std::fabs(inputValue1));
        auto env = envelopeFilter.processSample(0, maxValue);
        return std::pair<SampleType, SampleType> {env, env};
    }
    case ChannelAggregationType::mean:
    {
        auto meanValue =
            balFilterType == EnvCalculationType::peak ?
            0.5f * (std::fabs(inputValue0) + std::fabs(inputValue1)) :
            std::sqrt(0.5f * (inputValue0 * inputValue0 + inputValue1 * inputValue1));
        auto env = envelopeFilter.processSample(0, meanValue);
        return std::pair<SampleType, SampleType> {env, env};
    }
    default:
        jassertfalse;
    }
}

template<typename SampleType>
SampleType DynamicShaper<SampleType>::calculateGain(
    SampleType inputValue,
    SampleType envValue)
{
    if (size == 0)
        return inputValue;
    int i = findKneeIndex(envValue);
    if (i < 0)
        return gain[0] * inputValue;
    SampleType coeff;
    if (envValue >= kneeRightBound[i])
        coeff = std::pow(envValue * thresholdInverse[i], ratioInverseMinusOne[i]);
    else
    {
        SampleType envDb = juce::Decibels::gainToDecibels(envValue);
        SampleType envYDb = envDb * (envDb * aQuadCoeff[i] + bQuadCoeff[i]) + cQuadCoeff[i];
        coeff = juce::Decibels::decibelsToGain(envYDb - envDb);
    }
    return gain[i] * inputValue * coeff;
}

// private methods

template<typename SampleType>
int DynamicShaper<SampleType>::findKneeIndex(SampleType value)
{
    int res = -1;
    for (int i = 0; i < size; i++)
        res += (int)(value > kneeLeftBound[i]);
    return  res;
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
    threshold[kneeIndex] = juce::Decibels::decibelsToGain(newThreshold);
    thresholdInverse[kneeIndex] = 1.0 / threshold[kneeIndex];
    ratioInverseMinusOne[kneeIndex] = 1.0 / newRatio - 1.0;
    kneeLeftBoundDb[kneeIndex] = newThreshold - 0.5f * newKneeWidth;
    kneeLeftBound[kneeIndex] = juce::Decibels::decibelsToGain(kneeLeftBoundDb[kneeIndex], minusInfinityDb);
    kneeRightBound[kneeIndex] = juce::Decibels::decibelsToGain(newThreshold + 0.5f * newKneeWidth, minusInfinityDb);

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

//==============================================================================
template class DynamicShaper<float>;
template class DynamicShaper<double>;
