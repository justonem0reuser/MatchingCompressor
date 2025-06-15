#include "QuantilesCalculator.h"
#include <JuceHeader.h>
#include "../Messages.h"

std::vector<float> QuantilesCalculator::calculateQuantiles(
    std::vector<std::vector<float>>& input,
    int gainRegionsNumber,
    int quantilesNumber)
{
    auto numChannels = input.size();
    auto numSamples = input[0].size();
    auto size = numChannels * numSamples;
    if (size <= quantilesNumber)
        throw std::exception(numRegionsTooBigExStr.getCharPointer());

    if (size <= gainRegionsNumber * 100)
        return calculateQuantilesPrecise(input, quantilesNumber);
    else
    {
        auto density = calculateDensityFunc(input, gainRegionsNumber);
        return density2Quantiles(density, quantilesNumber, size);
    }
}

std::vector<float> QuantilesCalculator::calculateQuantilesPrecise(
    std::vector<std::vector<float>>& input, 
    int quantilesNumber)
{
    std::vector<float> gainStat;
    auto numChannels = input.size();
    auto numSamples = input[0].size();
    auto size = numSamples * numChannels;
    gainStat.resize(size);
    auto index = 0;
    for (int i = 0; i < numChannels; i++)
        for (int j = 0; j < numSamples; j++)
            gainStat[index++] = std::fabs(input[i][j]);
    std::sort(gainStat.begin(), gainStat.end());

    std::vector<float> res;
    res.resize(quantilesNumber);
    for (int i = 0; i < quantilesNumber; i++)
    {
        int index = static_cast<int>(size * (i + 0.5) / quantilesNumber);
        res[i] = gainStat[index];
    }
    return res;
}

std::vector<float> QuantilesCalculator::density2Quantiles(
    std::vector<int>& density,
    int size,
    int samplesNumber)
{
    std::vector<float> res;
    res.resize(size);
    const int densitySize = density.size();
    const float densityBeanWidth = 1.f / densitySize;
    const float qCoeff = samplesNumber / (size + 1.f);

    int regionIndex = 0;
    int cumulativeValue = 0;
    float quantileValue = (regionIndex + 1.f) * qCoeff;

    for (int i = 0; i < densitySize && regionIndex != size; i++)
    {
        int nextValue = cumulativeValue + density[i];
        while (nextValue >= quantileValue && regionIndex != size)
        {
            float leftGain = i * densityBeanWidth;
            res[regionIndex] = leftGain +
                densityBeanWidth * (quantileValue - cumulativeValue) / (nextValue - cumulativeValue);
            regionIndex++;
            quantileValue = (regionIndex + 1.f) * qCoeff; // recalculating to avoid precision errors
        }
        cumulativeValue = nextValue;
    }
    if (regionIndex != size)
    {
        float lastVal = res[regionIndex];
        while (regionIndex != size)
            res[regionIndex++] = lastVal;
    }
    return res;
}

std::vector<int> QuantilesCalculator::calculateDensityFunc(
    std::vector<std::vector<float>>& samples,
    int beanCount)
{
    std::vector<int> densFunc;
    float densFuncBeanWidth = 1.f / beanCount;
    densFunc.resize(beanCount);
    auto numChannels = samples.size();
    auto numSamples = samples[0].size();
    for (auto i = 0; i < numChannels; i++)
        for (auto j = 0; j < numSamples; j++)
        {
            int index = std::min((int)(std::fabs(samples[i][j]) * beanCount), beanCount - 1);
            densFunc[index]++;
        }
    return densFunc;
}
