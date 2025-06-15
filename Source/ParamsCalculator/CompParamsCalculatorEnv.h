#pragma once
#include "CompParamsCalculator.h"
#include "../DSP/DynamicShaper.h"
#include "HashEqualStructures.h"

using ChannelAggregationType = DynamicShaper<float>::ChannelAggregationType;

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
    bool isEnvelopeNeeded;
    EnvCalculationType balFilterType;
    ChannelAggregationType channelAggregationType;

    std::unordered_map<alglib::real_1d_array, std::vector<float>, Real1DArrayHash, Real1DArrayEqual> calculatedFunctions;

    DynamicShaper<float> dynamicProcessor;
    juce::dsp::ProcessSpec spec;

    virtual void calculateEnvelopeStatistics(
        std::vector<std::vector<float>>& samples,
        double sampleRate,
        float attackMs,
        float releaseMs) = 0;
    virtual std::vector<float> calculateFunction(
        std::vector<std::vector<float>>& samples,
        const alglib::real_1d_array& parameters) = 0;


    // for compressing if attack == release == 0
    std::vector<float> calculateNoEnvelopeFunction(
        std::vector<std::vector<float>>& input,
        const alglib::real_1d_array& params);

    void setCompParameters(const alglib::real_1d_array& params);

private:
    const double epsx = 0.00000001;
    const double diffstep = 0.025;
    const alglib::ae_int_t maxits = 0;
};