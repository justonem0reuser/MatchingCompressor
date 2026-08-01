#include "CompParamsCalculatorNoEnv.h"
#include "FuncAndGradCalculator.h"
#include "QuantilesCalculator.h"
#include "../Data/Messages.h"
#include "interpolation.h"

std::vector<float> CompParamsCalculatorNoEnv::calculateCompressorParameters(
    std::vector<std::vector<float>>& refSamples, 
    std::vector<std::vector<float>>& destSamples, 
    double destSampleRate,
    juce::ValueTree& properties)
{
    int gainRegNum = properties.getProperty(setGainRegionsNumberId);
    int quantRegNum = properties.getProperty(setQuantileRegionsNumberId);
    int kneeTypeInt = properties.getProperty(setKneeTypeId);
    float attackMs = properties.getProperty(setAttackId);
    float releaseMs = properties.getProperty(setReleaseId);
    int kneesNumber = properties.getProperty(setKneesNumberId);
    int channelAggregationTypeInt = properties.getProperty(setChannelAggregationTypeId);

    KneeType kneeType =
        kneeTypeInt == 1 ?
        KneeType::hard :
        KneeType::soft;
    
    jassert(attackMs == 0 && releaseMs == 0);
    jassert(destSamples.size() == 1 || channelAggregationTypeInt == 1);

    if (quantRegNum < 2)
        throw std::runtime_error(regionsNumberTooSmall.toStdString());
    std::vector<float> localReferenceStat, localDestStat;
    localReferenceStat = QuantilesCalculator::calculateQuantiles(refSamples, gainRegNum, quantRegNum);
    localDestStat = QuantilesCalculator::calculateQuantiles(destSamples, gainRegNum, quantRegNum);

    alglib::real_2d_array x;
    alglib::real_1d_array  bndl, bndu, y, c;
    alglib::lsfitstate state;
    alglib::lsfitreport rep;

    x.setlength(quantRegNum, 1);
    y.setlength(quantRegNum);
    for (int i = 0; i < quantRegNum; i++)
    {
        x[i][0] = localDestStat[i];
        y[i] = localReferenceStat[i];
    }

    setInitGuessAndBounds(kneesNumber, kneeType, c, bndl, bndu);

    try
    {
        lsfitcreatefg(x, y, c, true, state);
        lsfitsetcond(state, epsx, maxits);
        lsfitsetbc(state, bndl, bndu);
        lsfitfit(state, calculateFunctional, calculateGradient);
        lsfitresults(state, c, rep);
    }
    catch (const alglib::ap_error&)
    {
        throw std::runtime_error(cannotCalculateErrStr.toStdString());
    }

    if (rep.terminationtype < 0)
        throw std::runtime_error(cannotCalculateErrStr.toStdString());
    return resArrayToVector(c);
}

void CompParamsCalculatorNoEnv::calculateFunctional(
    const alglib::real_1d_array& c, 
    const alglib::real_1d_array& x, 
    double& func, 
    void* ptr)
{
    func = calculateFunctionalAndGradientWithoutFine(c, x) + calculateFine(c);
}

void CompParamsCalculatorNoEnv::calculateGradient(
    const alglib::real_1d_array& c, 
    const alglib::real_1d_array& x, 
    double& func, 
    alglib::real_1d_array& grad, 
    void* ptr)
{
    func = calculateFunctionalAndGradientWithoutFine(c, x, &grad) + calculateFine(c, &grad);
}

double CompParamsCalculatorNoEnv::calculateFunctionalAndGradientWithoutFine(
    const alglib::real_1d_array& c, 
    const alglib::real_1d_array& x, 
    alglib::real_1d_array* gradPtr)
{
    //c : Gain, [Threshold, 1/Ratio, Knee weight] * n
    double func = c[0] + FuncAndGradCalculator::calculateWithoutGain(
        juce::Decibels::gainToDecibels(x[0], minusInfinityDb),
        c,
        gradPtr);
    if (gradPtr != nullptr)
        (*gradPtr)[0] = 1.0;
    dbToGain(c.length(), func, gradPtr);
    return func;
}

void CompParamsCalculatorNoEnv::dbToGain(
    int cLength, 
    double& func, 
    alglib::real_1d_array* gradPtr)
{
    func = juce::Decibels::decibelsToGain(func, minusInfinityDb);
    if (gradPtr != nullptr)
    {
        const double coeff = std::log(std::pow(10.0, 0.05));
        for (int i = 0; i < cLength; i++)
            (*gradPtr)[i] *= func * coeff;
    }
}
