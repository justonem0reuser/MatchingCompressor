#pragma once
#include <vector>
#include "GranularityCalculator.h"
#include "QuantilesCalculator.h"

class ReferenceQuantiles
{
public:
    void build(
        std::vector<std::vector<float>>& samples,
        int gainRegionsNumber,
        int quantileRegionsNumber,
        int samplesNumber,
        float scale)
    {
        auto density = QuantilesCalculator::calculateDensityFunc(samples, gainRegionsNumber, scale);
        int nonEmptyBeansNumber = 0;
        for (double d : density)
            if (d > 0.0)
                nonEmptyBeansNumber++;
        quantiles = QuantilesCalculator::density2Quantiles(
            density,
            GranularityCalculator::capQuantilesNumberByOccupancy(
                quantileRegionsNumber,
                nonEmptyBeansNumber),
            samplesNumber);
    }

    const std::vector<float>& get() const { return quantiles; }

private:
    std::vector<float> quantiles;
};
