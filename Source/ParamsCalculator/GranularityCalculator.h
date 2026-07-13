#pragma once
#include <algorithm>

struct Granularity
{
    int gainRegions;
    int quantileRegions;
};

class GranularityCalculator
{
public:
    static inline Granularity calculateTargetGranularity(int kneesNumber, bool isKneeSoft)
    {
        const int shapeParams = (isKneeSoft ? 3 : 2) * kneesNumber;
        int quant = std::min(quantPerParam * shapeParams, quantMax);
        int grn =
            isKneeSoft ?
            gainRegionsSoft :
            std::min(resolutionCoeff * quant, gainRegionsMaxHard);
        quant = std::min(quant, grn / resolutionCoeff); // in case of private constants changing
        return { grn, quant };
    }

    static inline int capQuantilesNumberByOccupancy(int quantilesNumber, int occupiedBinsNumber)
    {
        const int cap = std::max(quantMin, occupiedBinsNumber / occupancyCoeff);
        return std::min(quantilesNumber, cap);
    }

    // The real min samples number should be even more:
    // it should contain dozens of independent windows, 
    // and the length of each window is max(attack, release).
    static inline int calculateMinSamplesNumber()
    {
        return gainRegionsSoft * minSamplesPerBin;
    }

private:
    static constexpr int quantPerParam = 50;
    static constexpr int quantMin = 32;
    static constexpr int quantMax = 400;
    static constexpr int resolutionCoeff = 5; // resolutionCoeff >= 3 for quadratic kernel
    static constexpr int gainRegionsMaxHard = 2000;
    static constexpr int gainRegionsSoft = 5000;
    static constexpr int minSamplesPerBin = 32;
    static constexpr int occupancyCoeff = 3;
};