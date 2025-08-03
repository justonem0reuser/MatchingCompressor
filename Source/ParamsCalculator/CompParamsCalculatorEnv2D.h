#pragma once
#include "CompParamsCalculatorEnv.h"

/// <summary>
/// Matching compressor parameters calculation realization
/// optimized for the case when overall sample count is much greater 
/// than the gain bean number.
/// </summary>
class CompParamsCalculatorEnv2D : public CompParamsCalculatorEnv
{
private:
    alglib::integer_2d_array xEnvTable;

    std::vector<float> calculateFunction(
        std::vector<std::vector<float>>& samples,
        const alglib::real_1d_array& parameters) override;

    void calculateEnvelopeStatistics(
        std::vector<std::vector<float>>& samples, 
        double sampleRate, 
        float attackMs, 
        float releaseMs) override;

    std::vector<int> calculateYDensity(const alglib::real_1d_array& params);
};
