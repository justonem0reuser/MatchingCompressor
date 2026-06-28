#include "QuantilesCalculator.h"
#include <JuceHeader.h>
#include "../Data/Messages.h"

std::vector<float> QuantilesCalculator::calculateQuantiles(
    std::vector<std::vector<float>>& input,
    int gainRegionsNumber,
    int quantilesNumber)
{
    auto numChannels = input.size();
    auto numSamples = input[0].size();
    auto size = numChannels * numSamples;
    if (size <= quantilesNumber)
        throw std::runtime_error(numRegionsTooBigExStr.toStdString());

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
    auto numChannels = input.size();
    auto numSamples = input[0].size();
    auto size = numSamples * numChannels;
    std::vector<float> gainStat(size);
    auto index = 0;
    for (int i = 0; i < numChannels; i++)
        for (int j = 0; j < numSamples; j++)
            gainStat[index++] = std::fabs(input[i][j]);
    std::sort(gainStat.begin(), gainStat.end());

    std::vector<float> res(quantilesNumber);
    for (int i = 0; i < quantilesNumber; i++)
    {
        int gainStatIndex = static_cast<int>(size * (i + 0.5) / quantilesNumber);
        res[i] = gainStat[gainStatIndex];
    }
    return res;
}

std::vector<float> QuantilesCalculator::density2Quantiles(
    std::vector<double>& density,
    int size,
    int samplesNumber)
{
    // The internal calculation is performed in double
    // to prevent accuracy loss for large values.
    // The result is casted to float.
    std::vector<float> res(size, 0.0f);
    const int densitySize = density.size();
    const double densityBeanWidth = 1.0 / densitySize;
    const double qCoeff = samplesNumber / (size + 1.0);

    int regionIndex = 0;
    double cumulativeValue = 0.0;
    double quantileValue = (regionIndex + 1.0) * qCoeff;

    for (int i = 0; i < densitySize && regionIndex != size; i++)
    {
        double nextValue = cumulativeValue + density[i];
        while (nextValue >= quantileValue && regionIndex != size)
        {
            double leftGain = i * densityBeanWidth;
            res[regionIndex] = (float)(leftGain +
                densityBeanWidth * (quantileValue - cumulativeValue) / (nextValue - cumulativeValue));
            regionIndex++;
            quantileValue = (regionIndex + 1.0) * qCoeff; // recalculating to avoid precision errors
        }
        cumulativeValue = nextValue;
    }
    jassert(regionIndex > 0 || regionIndex == size);
    if (regionIndex != size)
    {
        float lastVal = res[regionIndex - 1];
        while (regionIndex != size)
            res[regionIndex++] = lastVal;
    }
    return res;
}

void QuantilesCalculator::putToBeans(
    float value, std::vector<double>& beans, int weight)
{
    auto count = (int)beans.size();
    double index = std::clamp(
        std::fabs((double)value) * count - 0.5,
        0.0, count - 1.0);
    double leftBeanIndex = std::floor(index);
    double rightBeanPart = index - leftBeanIndex;
    int leftBeanIndexInt = (int)leftBeanIndex;
    beans[leftBeanIndexInt] += (1.0 - rightBeanPart) * weight;
    if (leftBeanIndexInt < count - 1)
        beans[leftBeanIndexInt + 1] += rightBeanPart * weight;
}

std::vector<double> QuantilesCalculator::calculateDensityFunc(
    std::vector<std::vector<float>>& samples,
    int beanCount)
{
    std::vector<double> densFunc(beanCount, 0.0);
    auto numChannels = samples.size();
    auto numSamples = samples[0].size();
    for (auto i = 0; i < numChannels; i++)
        for (auto j = 0; j < numSamples; j++)
            putToBeans(samples[i][j], densFunc, 1);
    return densFunc;
}
