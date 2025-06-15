#pragma once
#include <vector>

static class QuantilesCalculator
{
public:
    static std::vector<float> calculateQuantiles(
        std::vector<std::vector<float>>& input,
        int gainRegionsNumber,
        int quantilesNumber);
    
    static std::vector<float> density2Quantiles(
        std::vector<int>& density,
        int size,
        int samplesNumber);

private:
    static std::vector<float> calculateQuantilesPrecise(
        std::vector<std::vector<float>>& input,
        int quantilesNumber);

    static std::vector<int> calculateDensityFunc(
        std::vector<std::vector<float>>& samples,
        int beanCount);
};