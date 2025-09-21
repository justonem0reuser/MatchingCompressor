#include "CompParamsCalculatorEnv2D.h"
#include "QuantilesCalculator.h"

std::vector<float> CompParamsCalculatorEnv2D::calculateFunction(
    std::vector<std::vector<float>>& samples,
    const alglib::real_1d_array& parameters)
{
    auto samplesCount = samples.size() * samples[0].size();
    auto yDensity = calculateYDensity(parameters);
    return QuantilesCalculator::density2Quantiles(yDensity, quantileRegionsNumber, samplesCount);
}

void CompParamsCalculatorEnv2D::calculateEnvelopeStatistics(
    std::vector<std::vector<float>>& samples,
    double sampleRate,
    float attackMs,
    float releaseMs)
{
    xEnvTable.setlength(gainRegionsNumber, gainRegionsNumber);
    for (int i = 0; i < gainRegionsNumber; i++)
        for (int j = 0; j < gainRegionsNumber; j++)
            xEnvTable[i][j] = 0.;

    auto numChannels = samples.size();
    auto numSamples = samples[0].size();
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
            int i1 = std::min((int)(sAbs * gainRegionsNumber), gainRegionsNumber - 1);
            int i2 = std::min((int)(env * gainRegionsNumber), gainRegionsNumber - 1);
            xEnvTable[i1][i2]++;
        }
    }
    else
    {
        for (size_t i = 0; i < numSamples; i++)
        {
            float sample0 = samples[0][i];
            float sample1 = samples[1][i];
            float sAbs0 = std::fabsf(sample0);
            float sAbs1 = std::fabsf(sample1);
            auto env = dynamicProcessor.calculateStereoEnv(sample0, sample1);
            int i1 = std::min((int)(sAbs0 * gainRegionsNumber), gainRegionsNumber - 1);
            int i2 = std::min((int)(env.first * gainRegionsNumber), gainRegionsNumber - 1);
            xEnvTable[i1][i2]++;
            i1 = std::min((int)(sAbs1 * gainRegionsNumber), gainRegionsNumber - 1);
            i2 = std::min((int)(env.second * gainRegionsNumber), gainRegionsNumber - 1);
            xEnvTable[i1][i2]++;
        }
    }
    dynamicProcessor.reset();
}

std::vector<int> CompParamsCalculatorEnv2D::calculateYDensity(
    const alglib::real_1d_array& params)
{
    auto size = (int)xEnvTable.cols();
    float delta = 1.f / size;
    std::vector<int> res;
    res.resize(size);
    for (int i = 0; i < size; i++)
        res[i] = 0;
    setCompParameters(params);

    for (auto i = 0; i < size; i++)
    {
        float x = (i + 0.5f) * delta; // recalculate each step to increase precision
        for (auto j = 0; j < size; j++)
        {
            if (xEnvTable[i][j] != 0)
            {
                float env = (j + 0.5f) * delta;
                float y = dynamicProcessor.calculateGain(x, env);
                auto index = (int)(std::fabsf(y) * size);
                if (index < 0) // for overflow
                    index = size - 1;
                else
                    index = std::min((int)(std::fabsf(y) * size), size - 1);
                res[index] += xEnvTable[i][j];
            }
        }
    }
    return res;
}
