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

    auto density = calculateDensityFunc(input, gainRegionsNumber);
    return density2Quantiles(density, quantilesNumber, size);
}

std::vector<float> QuantilesCalculator::density2Quantiles(
    std::vector<double>& density,
    int size,
    int samplesNumber,
    std::vector<std::vector<double>>* dBins,
    std::vector<std::vector<double>>* jacobian)
{
    jassert((dBins == nullptr) == (jacobian == nullptr));

    // The internal calculation is performed in double
    // to prevent accuracy loss for large values.
    // The result is casted to float.
    std::vector<float> res(size, 0.0f);
    const int densitySize = density.size();
    const double densityBinWidth = 1.0 / densitySize;
    const double qCoeff = samplesNumber / (size + 1.0);

    int regionIndex = 0;
    double cumulativeValue = 0.0;
    double quantileValue = (regionIndex + 1.0) * qCoeff;
    const int paramsCount = dBins == nullptr ? 0 : (*dBins).size();
    std::vector<double> cumulativeDeriv(paramsCount, 0.0);
    std::vector<double> nextDeriv(paramsCount, 0.0);

    for (int i = 0; i < densitySize && regionIndex != size; i++)
    {
        const double di = density[i];
        const double nextValue = cumulativeValue + di;
        const double resCoeff = di == 0.0 ? 1.0 : densityBinWidth / di; // isn't used if di == 0.0
        const double jacCoeff = di == 0.0 ? 1.0 : resCoeff / di; // isn't used if di == 0.0
        for (int p = 0; p < paramsCount; p++)
            nextDeriv[p] = cumulativeDeriv[p] + (*dBins)[p][i];

        while (nextValue >= quantileValue && regionIndex != size)
        {
            double leftGain = i * densityBinWidth;
            res[regionIndex] = (float)(leftGain + resCoeff * (quantileValue - cumulativeValue));
            for (int p = 0; p < paramsCount; p++)
                (*jacobian)[p][regionIndex] =
                jacCoeff * (-cumulativeDeriv[p] * di - (quantileValue - cumulativeValue) * (*dBins)[p][i]);
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
    if (!(value >= 0.0)) // NaN and negatives
        value = 0.0; 
    else if (value > 1.0)
        value = 1.0;
    
    const double pos = value * binCount - 0.5;
    const int nearestBin = (int)std::floor(pos + 0.5);
    const double offset = pos - nearestBin;

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
    return nearestBin - 1;
}

void QuantilesCalculator::putToBins(
    double value, 
    std::vector<double>& bins, 
    int weight,
    std::vector<double>* grad,
    std::vector<std::vector<double>>* dBins)
{
    jassert((grad == nullptr) == (dBins == nullptr));
    jassert((grad == nullptr) ||
        ((*dBins).size() == (*grad).size() && (*dBins)[0].size() == bins.size()));

    const int count = (int)bins.size();
    double weights[kernelSupport], slopes[kernelSupport];
    const int leftBin = calculateKernelTaps(
        std::fabs(value), 
        count, 
        weights,
        grad ? slopes : nullptr);
    const int gradCount = grad != nullptr ? (int)(*grad).size() : 0;
    for (int i = 0; i < kernelSupport; i++)
    {
        const int bin = std::clamp(leftBin + i, 0, count - 1);
        bins[bin] += weights[i] * weight;
        for (int j = 0; j < gradCount; j++)
            (*dBins)[j][bin] += slopes[i] * (*grad)[j] * weight;
    }
}

std::vector<double> QuantilesCalculator::calculateDensityFunc(
    std::vector<std::vector<float>>& samples,
    int binCount)
{
    std::vector<double> densFunc(binCount, 0.0);
    auto numChannels = samples.size();
    auto numSamples = samples[0].size();
    for (auto i = 0; i < numChannels; i++)
        for (auto j = 0; j < numSamples; j++)
            putToBins(samples[i][j], densFunc, 1);
    return densFunc;
}
