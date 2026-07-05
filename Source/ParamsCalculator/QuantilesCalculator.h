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
        std::vector<std::vector<double>>* dBeans = nullptr,
        std::vector<std::vector<double>>* jacobian = nullptr);

    static void putToBeans(
        float value, 
        std::vector<double>& beans, 
        int weight,
        std::vector<float>* grad = nullptr,
        std::vector<std::vector<double>>* dBeans = nullptr);

    static bool usePreciseBranch(int size, int gainRegionsNumber);

    static std::vector<float> calculateQuantilesPrecise(
        std::vector<std::vector<float>>& input,
        int quantilesNumber,
        std::vector<int>* carriers = nullptr);

private:
    static constexpr int kernelSupport = 3; // quadratic B-spline

    static int calculateKernelTaps(
        double value, 
        int binCount, 
        double* weights, 
        double* slopes = nullptr);

    static std::vector<double> calculateDensityFunc(
        std::vector<std::vector<float>>& samples,
        int beanCount);
};
