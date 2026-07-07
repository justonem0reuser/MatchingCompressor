#include "CompParamsCalculatorEnv2D.h"
#include "QuantilesCalculator.h"
#include "FuncAndGradCalculator.h"

std::vector<float> CompParamsCalculatorEnv2D::calculateFunction(
    std::vector<std::vector<float>>& samples,
    const alglib::real_1d_array& parameters,
    std::vector<std::vector<double>>* jacobian)
{
    auto samplesCount = samples.size() * samples[0].size();
    std::vector<std::vector<double>> dBeans;
    std::vector<std::vector<double>>* dBeansPtr = nullptr;

    if (jacobian != nullptr)
    {
        const int parLength = parameters.length();
        const int beanCount = (int)xEnvTable.cols();
        dBeansPtr = &dBeans;
        dBeans.assign(parLength, std::vector<double>(beanCount, 0.0));
        jacobian->assign(parLength, std::vector<double>(quantileRegionsNumber, 0.0));
    }

    auto yDensity = calculateYDensity(parameters, dBeansPtr);
    return QuantilesCalculator::density2Quantiles(
        yDensity, quantileRegionsNumber, samplesCount, dBeansPtr, jacobian);
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

    const double delta = 1.0 / gainRegionsNumber;
    envDbByCol.resize(gainRegionsNumber);
    for (int j = 0; j < gainRegionsNumber; j++)
        envDbByCol[j] = juce::Decibels::gainToDecibels(
            (j + 0.5) * delta, DynamicShaper<double>::minusInfinityDb);

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
            float sAbs = std::fabs(sample);
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
            float sAbs0 = std::fabs(sample0);
            float sAbs1 = std::fabs(sample1);
            float out0, out1;
            dynamicProcessor.calculateStereoEnv(sample0, sample1, out0, out1);
            int i1 = std::min((int)(sAbs0 * gainRegionsNumber), gainRegionsNumber - 1);
            int i2 = std::min((int)(out0 * gainRegionsNumber), gainRegionsNumber - 1);
            xEnvTable[i1][i2]++;
            i1 = std::min((int)(sAbs1 * gainRegionsNumber), gainRegionsNumber - 1);
            i2 = std::min((int)(out1 * gainRegionsNumber), gainRegionsNumber - 1);
            xEnvTable[i1][i2]++;
        }
    }
    dynamicProcessor.reset();
}

std::vector<double> CompParamsCalculatorEnv2D::calculateYDensity(
    const alglib::real_1d_array& params,
    std::vector<std::vector<double>>* dBeans)
{
    auto size = (int)xEnvTable.cols();
    float delta = 1.f / size;
    std::vector<double> res(size, 0.0);

    const double scaleCoeff = 0.05 * std::log(10.0);
    const int n = params.length();
    std::vector<float> grad(n);
    alglib::real_1d_array gradDb;
    if (dBeans != nullptr)
        gradDb.setlength(n);

    for (auto i = 0; i < size; i++)
    {
        float x = (i + 0.5f) * delta; // recalculate each step to increase precision
        for (auto j = 0; j < size; j++)
        {
            auto weight = xEnvTable[i][j];
            if (weight == 0)
                continue;

            double envDb = envDbByCol[j];
            double yDb = FuncAndGradCalculator::calculateWithoutGain(
                envDb, params, false, dBeans != nullptr ? &gradDb : nullptr); // true would require division by envDb
            double yAbs = (double)x * juce::Decibels::decibelsToGain(yDb - envDb);

            if (dBeans == nullptr)
                QuantilesCalculator::putToBeans((float)yAbs, res, weight);
            else
            {
                double scale = yAbs * scaleCoeff;
                for (int p = 1; p < n; p++) // p = 0 is gain
                    grad[p] = (float)(scale * gradDb[p]);
                QuantilesCalculator::putToBeans((float)yAbs, res, weight, &grad, dBeans);
            }
        }
    }
    return res;
}
