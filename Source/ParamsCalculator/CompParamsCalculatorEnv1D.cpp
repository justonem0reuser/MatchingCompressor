#include "CompParamsCalculatorEnv1D.h"
#include "QuantilesCalculator.h"
#include "FuncAndGradCalculator.h"

std::vector<float> CompParamsCalculatorEnv1D::calculateFunction(
    std::vector<std::vector<float>>& samples,
    const alglib::real_1d_array& parameters,
    std::vector<std::vector<double>>* jacobian)
{
    const int size = (int)xEnvPairs.size();
    const int parLength = parameters.length();
    std::vector<std::vector<float>> y(1, std::vector<float>(size));
    const double scaleCoeff = 0.05 * std::log(10.0);
    std::vector<std::vector<float>> grad;
    alglib::real_1d_array gradDb;
    if (jacobian != nullptr)
    {
        grad.assign(size, std::vector<float>(parLength, 0.0f));
        gradDb.setlength(parLength);
        jacobian->assign(parLength, std::vector<double>(quantileRegionsNumber, 0.0));
    }

    for (auto i = 0; i < size; i++)
    {
        float x = xEnvPairs[i].xLinear;
        double envDb = xEnvPairs[i].envDb;
        double yDb = FuncAndGradCalculator::calculateWithoutGain(
            envDb, 
            parameters, 
            false, 
            jacobian == nullptr ? nullptr : &gradDb);
        double yAbs = (double)x * juce::Decibels::decibelsToGain(yDb - envDb);
        y[0][i] = (float)yAbs;
        if (jacobian != nullptr)
        {
            double scale = yAbs * scaleCoeff;
            for (int p = 1; p < parLength; p++) // grad[i][0] is gain
                grad[i][p] = (float)(scale * gradDb[p]);
        }
    }

    if (jacobian == nullptr)
        return QuantilesCalculator::calculateQuantiles(y, gainRegionsNumber, quantileRegionsNumber);

    if (QuantilesCalculator::usePreciseBranch(size, gainRegionsNumber))
    {
        std::vector<int> carriers;
        auto quantiles = QuantilesCalculator::calculateQuantilesPrecise(
            y, 
            quantileRegionsNumber, 
            &carriers);
        for (int k = 0; k < quantileRegionsNumber; k++)
            for (int p = 0; p < parLength; p++) // grad[i][0] is gain
                (*jacobian)[p][k] = grad[carriers[k]][p];
        return quantiles;
    }

    std::vector<double> density(gainRegionsNumber, 0.0);
    std::vector<std::vector<double>> dBeans(parLength, std::vector<double>(gainRegionsNumber, 0.0));
    for (int i = 0; i < size; i++)
        QuantilesCalculator::putToBeans(
            y[0][i], 
            density, 
            1, 
            &grad[i], 
            &dBeans);
    return QuantilesCalculator::density2Quantiles(
        density, 
        quantileRegionsNumber, 
        size, 
        &dBeans, 
        jacobian);
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
            float sAbs = std::fabs(sample);
            float env = dynamicProcessor.calculateEnv(0, sample);
            float envDb = juce::Decibels::gainToDecibels(
                env, DynamicShaper<float>::minusInfinityDb);
            xEnvPairs[i] = { sAbs, envDb };
        }
    }
    else
    {
        auto index = 0;
        for (size_t i = 0; i < numSamples; i++)
        {
            float sample0 = samples[0][i];
            float sample1 = samples[1][i];
            float sAbs0 = std::fabs(sample0);
            float sAbs1 = std::fabs(sample1);
            float out0, out1;
            dynamicProcessor.calculateStereoEnv(sample0, sample1, out0, out1);
            float envDb0 = juce::Decibels::gainToDecibels(
                out0, DynamicShaper<float>::minusInfinityDb);
            float envDb1 = juce::Decibels::gainToDecibels(
                out1, DynamicShaper<float>::minusInfinityDb);
            xEnvPairs[index++] = { sAbs0, envDb0 };
            xEnvPairs[index++] = { sAbs1, envDb1 };
        }
    }
    dynamicProcessor.reset();
}