#pragma once
#include "CompParamsCalculatorEnv.h"

class CompParamsCalculatorEnv1D : public CompParamsCalculatorEnv
{
private:
    std::vector<std::pair<float, float>> xEnvPairs;

    std::vector<float> calculateFunction(
        std::vector<std::vector<float>>& samples,
        const alglib::real_1d_array& parameters) override;

    void calculateEnvelopeStatistics(
        std::vector<std::vector<float>>& samples, 
        double sampleRate, 
        float attackMs, 
        float releaseMs) override;
};
