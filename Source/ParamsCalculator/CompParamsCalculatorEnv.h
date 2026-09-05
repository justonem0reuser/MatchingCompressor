#pragma once
#include <cstdint>
#include "CompParamsCalculator.h"
#include "../DSP/DynamicShaper.h"
#include "EnvelopeHistogram.h"
#include "ReferenceQuantiles.h"
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

    void prepare(
        std::vector<std::vector<float>>& refSamples,
        std::vector<std::vector<float>>& destSamples,
        double refSampleRate,
        juce::ValueTree& properties);

    void prepareStructure(
        std::vector<std::vector<float>>& refSamples,
        const juce::ValueTree& properties);

    void prepareForFixation(
        std::vector<std::vector<float>>& destSamples,
        double destSampleRate,
        juce::ValueTree& properties);

    void updateBallistics(float attackMs, float releaseMs, bool isSoftNeeded = true);

    void updateEnvSettings(int balFilterTypeInt, int channelAggregationTypeInt);

    std::vector<float> solve(); // fits the reference target built by prepare()

    std::vector<float> solve(
        const std::vector<float>& target,
        const std::vector<float>& targetSoft,
        const std::vector<float>* warmStart = nullptr);

    // prepare()+updateBallistics() must be called before it.
    std::vector<float> calculateQuantilesFor(const std::vector<float>& params);

    void captureNominalKneeWidths(const std::vector<float>& params);

    float scoreAgainstReference(const std::vector<float>& params);

private:
    struct FunctionAndJacobian
    {
        std::vector<float> q; // quantiles
        std::vector<std::vector<double>> jac; // [param][quantile]
        bool hasJacobian = false;
    };

    const double epsx = 0.001;
    const alglib::ae_int_t maxits = 0;
    
    std::vector<std::vector<float>> destSamples;
    int gainRegionsNumber, gainRegionsNumberSoft;
    int quantileRegionsNumber, quantileRegionsNumberSoft;
    EnvCalculationType balFilterType;
    ChannelAggregationType channelAggregationType;
    KneeType kneeType = KneeType::hard;
    int kneesNumber = 1;
    double sampleRate = 0.;
    float maxAmp = 1.f;
    ReferenceQuantiles referenceQuantiles, referenceQuantilesSoft;

    /// <summary>
    /// Container for storing and reusing functional calculation results.
    /// </summary>
    std::unordered_map<alglib::real_1d_array, FunctionAndJacobian, Real1DArrayHash, Real1DArrayEqual> calculatedFunctions;

    DynamicShaper<float> dynamicProcessor;
    juce::dsp::ProcessSpec spec;

    EnvelopeHistogram histogram, histogramSoft;
    const EnvelopeHistogram* activeHistogram = &histogram;

    // Non-empty while the knee widths follow the thresholds instead of being optimized.
    std::vector<double> nominalKneeWidths, kneeWidths, dKneeWidthDThreshold;

    std::vector<double> fixationNominalWidths;

    void updateKneeWidths(const alglib::real_1d_array& c);

    std::vector<double> calculateYDensity(
        const alglib::real_1d_array& params,
        std::vector<std::vector<double>>* dBins = nullptr);

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
        float releaseMs,
        bool isSoftNeeded);
    std::vector<float> calculateFunction(
        std::vector<std::vector<float>>& samples,
        const alglib::real_1d_array& parameters,
        std::vector<std::vector<double>>* jacobian = nullptr);

    std::vector<float>& getY(const alglib::real_1d_array& c);
    FunctionAndJacobian& getYAndJ(const alglib::real_1d_array& c);

    void paramsToC(const std::vector<float>& params, alglib::real_1d_array& c, bool isWidthVariable);

    void configure(const juce::ValueTree& properties);

    float fitMismatchAt(const alglib::real_1d_array& c, const std::vector<float>& target);
};