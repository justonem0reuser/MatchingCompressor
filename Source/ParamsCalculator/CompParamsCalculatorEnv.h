#pragma once
#include "CompParamsCalculator.h"
#include "../DSP/DynamicShaper.h"
#include "HashEqualStructures.h"

using ChannelAggregationType = DynamicShaper<float>::ChannelAggregationType;

/// <summary>
/// Base matching compressor parameters calculation class
/// for the case when the processing result
/// depends not only on the actual sample,
/// but also on previous or other channels samples.
/// </summary>
class CompParamsCalculatorEnv : public CompParamsCalculator
{
public:
    std::vector<float> calculateCompressorParameters(
        std::vector<std::vector<float>>& refSamples, 
        double refSampleRate,
        std::vector<std::vector<float>>& destSamples, 
        double destSampleRate,
        juce::ValueTree& properties) override;

    std::vector<float>& getY(const alglib::real_1d_array& c);

protected:
    std::vector<std::vector<float>> destSamples;
    int gainRegionsNumber;
    int quantileRegionsNumber;
    EnvCalculationType balFilterType;
    ChannelAggregationType channelAggregationType;

    /// <summary>
    /// Container for storing and reusing functional calculation results.
    /// </summary>
    std::unordered_map<alglib::real_1d_array, std::vector<float>, Real1DArrayHash, Real1DArrayEqual> calculatedFunctions;

    DynamicShaper<float> dynamicProcessor;
    juce::dsp::ProcessSpec spec;

    static void calculateFunctional(
        const alglib::real_1d_array& c,
        const alglib::real_1d_array& x,
        double& func,
        void* ptr);

    virtual void calculateEnvelopeStatistics(
        std::vector<std::vector<float>>& samples,
        double sampleRate,
        float attackMs,
        float releaseMs) = 0;
    virtual std::vector<float> calculateFunction(
        std::vector<std::vector<float>>& samples,
        const alglib::real_1d_array& parameters) = 0;

    void setCompParameters(const alglib::real_1d_array& params);

private:
    const double epsx = 0.00000001;
    const double diffstep = 0.025;
    const alglib::ae_int_t maxits = 0;
};