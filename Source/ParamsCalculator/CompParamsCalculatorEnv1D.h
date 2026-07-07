#pragma once
#include "CompParamsCalculatorEnv.h"

/// <summary>
/// Matching compressor parameters calculation realization
/// optimized for the case when overall sample count is small 
/// comparing to the gain bean number.
/// </summary>
class CompParamsCalculatorEnv1D : public CompParamsCalculatorEnv
{
private:
    struct XEnvPair { float xLinear; float envDb; };
    std::vector<XEnvPair> xEnvPairs;

    std::vector<float> calculateFunction(
        std::vector<std::vector<float>>& samples,
        const alglib::real_1d_array& parameters,
        std::vector<std::vector<double>>* jacobian) override;

    void calculateEnvelopeStatistics(
        std::vector<std::vector<float>>& samples, 
        double sampleRate, 
        float attackMs, 
        float releaseMs) override;
};
