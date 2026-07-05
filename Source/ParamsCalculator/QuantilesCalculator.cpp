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

    if (usePreciseBranch(size, gainRegionsNumber))
        return calculateQuantilesPrecise(input, quantilesNumber);
    else
    {
        auto density = calculateDensityFunc(input, gainRegionsNumber);
        return density2Quantiles(density, quantilesNumber, size);
    }
}

std::vector<float> QuantilesCalculator::calculateQuantilesPrecise(
    std::vector<std::vector<float>>& input, 
    int quantilesNumber,
    std::vector<int>* carriers)
{
    auto numChannels = input.size();
    auto numSamples = input[0].size();
    auto size = numSamples * numChannels;
    std::vector<float> res(quantilesNumber);

    if (carriers == nullptr) // value-only
    {
        std::vector<float> gainStat(size);
        auto index = 0;
        for (int i = 0; i < numChannels; i++)
            for (int j = 0; j < numSamples; j++)
                gainStat[index++] = std::fabs(input[i][j]);
        std::sort(gainStat.begin(), gainStat.end());

        for (int i = 0; i < quantilesNumber; i++)
        {
            int gainStatIndex = (int)(size * (i + 0.5) / quantilesNumber);
            res[i] = gainStat[gainStatIndex];
        }
    }
    else // gradient-aware
    {
        std::vector<std::pair<float, int>> gainStat(size);
        int index = 0;
        for (int i = 0; i < numChannels; i++)
            for (int j = 0; j < numSamples; j++)
            {
                gainStat[index] = { std::fabs(input[i][j]), index };
                index++;
            }
        std::sort(
            gainStat.begin(), gainStat.end(),
            [](const std::pair<float, int>& a, const std::pair<float, int>& b)
            {
                return a.first < b.first;
            });
        carriers->resize(quantilesNumber);
        for (int i = 0; i < quantilesNumber; i++)
        {
            int gainStatIndex = (int)(size * (i + 0.5) / quantilesNumber);
            res[i] = gainStat[gainStatIndex].first;
            (*carriers)[i] = gainStat[gainStatIndex].second;
        }
    }
    return res;
}

std::vector<float> QuantilesCalculator::density2Quantiles(
    std::vector<double>& density,
    int size,
    int samplesNumber,
    std::vector<std::vector<double>>* dBeans,
    std::vector<std::vector<double>>* jacobian)
{
    jassert((dBeans == nullptr) == (jacobian == nullptr));

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
    const int paramsCount = dBeans == nullptr ? 0 : (*dBeans).size();
    std::vector<double> cumulativeDeriv(paramsCount, 0.0);
    std::vector<double> nextDeriv(paramsCount, 0.0);

    for (int i = 0; i < densitySize && regionIndex != size; i++)
    {
        const double di = density[i];
        const double nextValue = cumulativeValue + di;
        const double resCoeff = di == 0.0 ? 1.0 : densityBeanWidth / di; // isn't used if di == 0.0
        const double jacCoeff = di == 0.0 ? 1.0 : resCoeff / di; // isn't used if di == 0.0
        for (int p = 0; p < paramsCount; p++)
            nextDeriv[p] = cumulativeDeriv[p] + (*dBeans)[p][i];

        while (nextValue >= quantileValue && regionIndex != size)
        {
            double leftGain = i * densityBeanWidth;
            res[regionIndex] = (float)(leftGain + resCoeff * (quantileValue - cumulativeValue));
            for (int p = 0; p < paramsCount; p++)
                (*jacobian)[p][regionIndex] =
                jacCoeff * (-cumulativeDeriv[p] * di - (quantileValue - cumulativeValue) * (*dBeans)[p][i]);
            regionIndex++;
            quantileValue = (regionIndex + 1.0) * qCoeff; // recalculating to avoid precision errors
        }
        cumulativeValue = nextValue;
        for (int p = 0; p < paramsCount; p++)
            cumulativeDeriv[p] = nextDeriv[p];
    }

    jassert(regionIndex > 0 || regionIndex == size);
    for (int i = regionIndex; i < size; i++) // if regionIndex != size
    {
        res[i] = res[regionIndex - 1];
        for (int p = 0; p < paramsCount; p++)
            (*jacobian)[p][i] = (*jacobian)[p][regionIndex - 1];
    }
    return res;
}

int QuantilesCalculator::calculateKernelTaps(
    double value, 
    int binCount, 
    double* weights, 
    double* slopes)
{
    const double pos = value * binCount - 0.5;
    const int nearestBean = (int)std::floor(pos + 0.5);
    const double offset = pos - nearestBean;

    // Quadratic B-spline
    weights[0] = 0.5 * (0.5 - offset) * (0.5 - offset);
    weights[1] = 0.75 - offset * offset;
    weights[2] = 0.5 * (0.5 + offset) * (0.5 + offset);

    if (slopes != nullptr)
    {
        slopes[0] = (offset - 0.5) * binCount;
        slopes[1] = (-2.0 * offset) * binCount;
        slopes[2] = (offset + 0.5) * binCount;
    }
    return nearestBean - 1;
}

void QuantilesCalculator::putToBeans(
    float value, 
    std::vector<double>& beans, 
    int weight,
    std::vector<float>* grad,
    std::vector<std::vector<double>>* dBeans)
{
    jassert((grad == nullptr) == (dBeans == nullptr));
    jassert((grad == nullptr) ||
        ((*dBeans).size() == (*grad).size() && (*dBeans)[0].size() == beans.size()));

    const int count = (int)beans.size();
    double weights[kernelSupport], slopes[kernelSupport];
    const int leftBean = calculateKernelTaps(
        std::fabs((double)value), 
        count, 
        weights,
        grad ? slopes : nullptr);
    const int gradCount = grad != nullptr ? (int)(*grad).size() : 0;
    for (int i = 0; i < kernelSupport; i++)
    {
        const int bin = std::clamp(leftBean + i, 0, count - 1);
        beans[bin] += weights[i] * weight;
        for (int j = 0; j < gradCount; j++)
            (*dBeans)[j][bin] += slopes[i] * (double)(*grad)[j] * weight;
    }
}

bool QuantilesCalculator::usePreciseBranch(int size, int gainRegionsNumber)
{
    return size <= gainRegionsNumber * 100;
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
