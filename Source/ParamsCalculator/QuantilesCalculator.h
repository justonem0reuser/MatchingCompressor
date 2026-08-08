#pragma once
#include <vector>

class QuantilesCalculator
{
public:
    static std::vector<float> calculateQuantiles(
        std::vector<std::vector<float>>& input,
        int gainRegionsNumber,
        int quantilesNumber);
    
    static std::vector<float> density2Quantiles(
        std::vector<double>& density,
        int size,
        int samplesNumber,
        std::vector<std::vector<double>>* dBins = nullptr,
        std::vector<std::vector<double>>* jacobian = nullptr);

    static void putToBins(
        double value, 
        std::vector<double>& bins, 
        int weight,
        std::vector<double>* grad = nullptr,
        std::vector<std::vector<double>>* dBins = nullptr);

    static std::vector<double> calculateDensityFunc(
        std::vector<std::vector<float>>& samples,
        int binCount);

private:
    static constexpr int kernelSupport = 3; // quadratic B-spline

    static int calculateKernelTaps(
        double value, 
        int binCount, 
        double* weights, 
        double* slopes = nullptr);
};
