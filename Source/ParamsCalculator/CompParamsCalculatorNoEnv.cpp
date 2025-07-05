#include "CompParamsCalculatorNoEnv.h"
#include "QuantilesCalculator.h"
#include "../Data/Messages.h"
#include "FuncAndGrad.h"
#include "interpolation.h"

std::vector<float> CompParamsCalculatorNoEnv::calculateCompressorParameters(
    std::vector<std::vector<float>>& refSamples, 
    double refSampleRate,
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
        throw std::exception(regionsNumberTooSmall.getCharPointer());
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

    lsfitcreatefg(x, y, c, true, state);
    lsfitsetcond(state, epsx, maxits);
    lsfitsetbc(state, bndl, bndu);
    lsfitfit(state, FuncAndGrad::comp_func_db, FuncAndGrad::comp_grad_db);
    lsfitresults(state, c, rep);

    if (rep.terminationtype < 0)
        throw std::exception(cannotCalculateErrStr.getCharPointer());
    return resArrayToVector(c);
}
