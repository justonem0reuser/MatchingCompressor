#pragma once

#include <JuceHeader.h>
#include "CompParamsCalculator.h"
#include "../DSP/DynamicShaper.h"

/// <summary>
/// Matching compressor parameters calculation class
/// for the case when the processing result
/// depends only on the actual sample,
/// not on previous or other channels samples.
/// </summary>
class CompParamsCalculatorNoEnv : public CompParamsCalculator
{
public:
    std::vector<float> calculateCompressorParameters(
        std::vector<std::vector<float>>& refSamples, 
        double refSampleRate,
        std::vector<std::vector<float>>& destSamples, 
        double destSampleRate,
        juce::ValueTree& properties) override;

private:
    const double epsx = 0.000001;
    const alglib::ae_int_t maxits = 0;
    constexpr static double minusInfinityDb = DynamicShaper<double>::minusInfinityDb;

    static void calculateFunctional(
        const alglib::real_1d_array& c,
        const alglib::real_1d_array& x,
        double& func,
        void* ptr);
    static void calculateGradient(
        const alglib::real_1d_array& c,
        const alglib::real_1d_array& x,
        double& func,
        alglib::real_1d_array& grad,
        void* ptr);
    static double calculateFunctionalAndGradientWithoutFine(
        const alglib::real_1d_array& c,
        const alglib::real_1d_array& x,
        alglib::real_1d_array* gradPtr = nullptr);
    static void dbToGain(
        int cLength,
        double& func,
        alglib::real_1d_array* gradPtr = nullptr);
};