#pragma once
#include "CompParamsCalculatorEnv.h"

/// <summary>
/// Matching compressor parameters calculation realization
/// optimized for the case when overall sample count is much greater 
/// than the gain bean number.
/// </summary>
class CompParamsCalculatorEnv2D : public CompParamsCalculatorEnv
{
protected:
    std::vector<float> calculateFunction(
        std::vector<std::vector<float>>& samples,
        const alglib::real_1d_array& parameters,
        std::vector<std::vector<double>>* jacobian) override;

    void calculateEnvelopeStatistics(
        std::vector<std::vector<float>>& samples, 
        double sampleRate, 
        float attackMs, 
        float releaseMs) override;

private:
    alglib::integer_2d_array xEnvTable;

    // envDbByCol[j] = envelope (in dB) at the center of grid column j.
    std::vector<double> envDbByCol;

    std::vector<double> calculateYDensity(
        const alglib::real_1d_array& params,
        std::vector<std::vector<double>>* dBeans = nullptr);
};
