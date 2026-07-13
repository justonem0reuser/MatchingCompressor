#pragma once
#include "CompParamsCalculator.h"
#include "../DSP/DynamicShaper.h"
#include "HashEqualStructures.h"

using ChannelAggregationType = DynamicShaper<float>::ChannelAggregationType;

/// <summary>
/// Matching compressor parameters calculation class
/// for the case when the processing result
/// depends not only on the actual sample,
/// but also on previous or other channels samples.
/// </summary>
class CompParamsCalculatorEnv : public CompParamsCalculator
{
public:
    std::vector<float> calculateCompressorParameters(
        std::vector<std::vector<float>>& refSamples, 
        std::vector<std::vector<float>>& destSamples, 
        double destSampleRate,
        juce::ValueTree& properties) override;

private:
    struct FunctionAndJacobian
    {
        std::vector<float> q; // quantiles + fine
        std::vector<std::vector<double>> jac; // [param][quantile] + grad(fine)
        bool hasJacobian = false;
    };

    const double epsx = 0.001;
    const alglib::ae_int_t maxits = 0;
    
    std::vector<std::vector<float>> destSamples;
    int gainRegionsNumber, gainRegionsNumberSoft;
    int quantileRegionsNumber, quantileRegionsNumberSoft;
    EnvCalculationType balFilterType;
    ChannelAggregationType channelAggregationType;

    /// <summary>
    /// Container for storing and reusing functional calculation results.
    /// </summary>
    std::unordered_map<alglib::real_1d_array, FunctionAndJacobian, Real1DArrayHash, Real1DArrayEqual> calculatedFunctions;

    DynamicShaper<float> dynamicProcessor;
    juce::dsp::ProcessSpec spec;

    alglib::integer_2d_array xEnvTable, xEnvTableSoft;

    // envDbByCol[j] = envelope (in dB) at the center of grid column j.
    std::vector<double> envDbByCol, envDbByColSoft;

    alglib::integer_2d_array* activeEnvTable = &xEnvTable;
    std::vector<double>* activeEnvDbByCol = &envDbByCol;

    std::vector<double> calculateYDensity(
        const alglib::real_1d_array& params,
        std::vector<std::vector<double>>* dBeans = nullptr);

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

    void calculateEnvelopeStatistics(
        std::vector<std::vector<float>>& samples,
        double sampleRate,
        float attackMs,
        float releaseMs);
    std::vector<float> calculateFunction(
        std::vector<std::vector<float>>& samples,
        const alglib::real_1d_array& parameters,
        std::vector<std::vector<double>>* jacobian = nullptr);

    std::vector<float>& getY(const alglib::real_1d_array& c);
    FunctionAndJacobian& getYAndJ(const alglib::real_1d_array& c);

    void addFine(
        const alglib::real_1d_array& c,
        FunctionAndJacobian& fj,
        bool withJacobian);
    
    void setCompParameters(const alglib::real_1d_array& params);
};