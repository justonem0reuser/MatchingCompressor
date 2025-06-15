#include "CompParamsCalculatorEnv1D.h"
#include "QuantilesCalculator.h"

std::vector<float> CompParamsCalculatorEnv1D::calculateFunction(
    std::vector<std::vector<float>>& samples,
    const alglib::real_1d_array& parameters)
{
    std::vector<std::vector<float>> y;
    auto size = xEnvPairs.size();
    y.push_back(std::vector<float>{});
    y[0].resize(size);
    setCompParameters(parameters);

    for (auto i = 0; i < size; i++)
        y[0][i] = dynamicProcessor.calculateGain(xEnvPairs[i].first, xEnvPairs[i].second);
    return QuantilesCalculator::calculateQuantiles(y, gainRegionsNumber, quantileRegionsNumber);
}

void CompParamsCalculatorEnv1D::calculateEnvelopeStatistics(
    std::vector<std::vector<float>>& samples,
    double sampleRate,
    float attackMs,
    float releaseMs)
{
    auto numChannels = samples.size();
    auto numSamples = samples[0].size();
    auto size = numChannels * numSamples;
    xEnvPairs.resize(size);

    spec.numChannels = numChannels;
    spec.sampleRate = sampleRate;
    dynamicProcessor.setEnvParameters(
        attackMs,
        releaseMs,
        balFilterType,
        channelAggregationType);
    dynamicProcessor.prepare(spec);

    if (numChannels == 1)
    {
        for (size_t i = 0; i < numSamples; i++)
        {
            float sample = samples[0][i];
            float sAbs = std::fabsf(sample);
            float env = dynamicProcessor.calculateEnv(0, sample);
            xEnvPairs[i] = { sAbs, env };
        }
    }
    else
    {
        auto index = 0;
        for (size_t i = 0; i < numSamples; i++)
        {
            float sample0 = samples[0][i];
            float sample1 = samples[1][i];
            float sAbs0 = std::fabsf(sample0);
            float sAbs1 = std::fabsf(sample1);
            auto env = dynamicProcessor.calculateStereoEnv(sample0, sample1);
            xEnvPairs[index++] = { sAbs0, env.first };
            xEnvPairs[index++] = { sAbs1, env.second };
        }
    }
    dynamicProcessor.reset();
}