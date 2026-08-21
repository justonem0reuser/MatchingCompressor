#include "CompParamsCalculatorNoEnv.h"
#include "GranularityCalculator.h"
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
    int kneeTypeInt = properties.getProperty(setKneeTypeId);
    float attackMs = properties.getProperty(setAttackId);
    float releaseMs = properties.getProperty(setReleaseId);
    kneesNumber = properties.getProperty(setKneesNumberId);
    int channelAggregationTypeInt = properties.getProperty(setChannelAggregationTypeId);

    KneeType kneeType =
        kneeTypeInt == 1 ?
        KneeType::hard :
        KneeType::soft;
    
    jassert(attackMs == 0 && releaseMs == 0);
    jassert(destSamples.size() == 1 || channelAggregationTypeInt == 1);

    const int allDestSamplesNumber = destSamples.size() * destSamples[0].size();
    auto g = GranularityCalculator::calculateTargetGranularity(
        kneesNumber, 
        kneeType == KneeType::soft);

    std::vector<std::vector<float>> refNormalized, destNormalized;
    const float maxAmp = normalize(
        refSamples, 
        destSamples, 
        refNormalized, 
        destNormalized);

    std::vector<float> localReferenceStat, localDestStat;
    localReferenceStat = QuantilesCalculator::calculateQuantiles(
        refNormalized, 
        g.gainRegions, 
        g.quantileRegions);
    localDestStat = QuantilesCalculator::calculateQuantiles(
        destNormalized, 
        g.gainRegions, 
        g.quantileRegions);

    alglib::real_2d_array x;
    alglib::real_1d_array  bndl, bndu, y, c;
    alglib::lsfitstate state;
    alglib::lsfitreport rep;

    x.setlength(g.quantileRegions, 1);
    y.setlength(g.quantileRegions);
    for (int i = 0; i < g.quantileRegions; i++)
    {
        x[i][0] = localDestStat[i];
        y[i] = localReferenceStat[i];
    }

    const bool isWidthVariable = (kneeType == KneeType::soft);
    setInitGuessAndBounds(kneesNumber, c, bndl, bndu, isWidthVariable);
    kneeWidths.clear();
    dKneeWidthDThreshold.clear();
    if (!isWidthVariable)
    {
        kneeWidths.assign(kneesNumber, 0.0);
        dKneeWidthDThreshold.assign(kneesNumber, 0.0);
    }

    try
    {
        lsfitcreatefg(x, y, c, true, state);
        lsfitsetcond(state, epsx, maxits);
        lsfitsetbc(state, bndl, bndu);
        setKneeConstraints(state, kneesNumber, isWidthVariable);
        lsfitfit(state, calculateFunctional, calculateGradient, nullptr, this);
        lsfitresults(state, c, rep);
    }
    catch (const alglib::ap_error&)
    {
        throw std::runtime_error(cannotCalculateErrStr.toStdString());
    }

    if (rep.terminationtype < 0)
        throw std::runtime_error(cannotCalculateErrStr.toStdString());

    enforceKneeGaps(c, kneesNumber, isWidthVariable);

    {
        double sumSq = 0.0;
        alglib::real_1d_array xi;
        xi.setlength(1);
        for (int i = 0; i < g.quantileRegions; i++)
        {
            xi[0] = localDestStat[i];
            double model = calculateFunctionalAndGradient(c, xi);
            double res = model - localReferenceStat[i];
            sumSq += res * res;
        }
        lastFitMismatch = fitMismatch(std::sqrt(sumSq / std::max(1, g.quantileRegions)), localReferenceStat);
    }

    if (!isWidthVariable)
        insertKneeWidths(c, kneesNumber, kneeWidths.data());

    auto result = resArrayToVector(c);
    denormalize(result, maxAmp);
    return result;
}

void CompParamsCalculatorNoEnv::calculateFunctional(
    const alglib::real_1d_array& c, 
    const alglib::real_1d_array& x, 
    double& func, 
    void* ptr)
{
    func = ((const CompParamsCalculatorNoEnv*)ptr)->calculateFunctionalAndGradient(c, x);
}

void CompParamsCalculatorNoEnv::calculateGradient(
    const alglib::real_1d_array& c, 
    const alglib::real_1d_array& x, 
    double& func, 
    alglib::real_1d_array& grad, 
    void* ptr)
{
    func = ((const CompParamsCalculatorNoEnv*)ptr)->calculateFunctionalAndGradient(c, x, &grad);
}

double CompParamsCalculatorNoEnv::calculateFunctionalAndGradient(
    const alglib::real_1d_array& c, 
    const alglib::real_1d_array& x, 
    alglib::real_1d_array* gradPtr) const
{
    //c : Gain, [Threshold, 1/Ratio (, Knee weight)] * n
    double func = c[0] + FuncAndGradCalculator::calculateWithoutGain(
        juce::Decibels::gainToDecibels(x[0], minusInfinityDb),
        c.getcontent(),
        kneesNumber,
        false,
        gradPtr != nullptr ? gradPtr->getcontent() : nullptr,
        kneeWidths.empty() ? nullptr : kneeWidths.data(),
        kneeWidths.empty() ? nullptr : dKneeWidthDThreshold.data());
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
