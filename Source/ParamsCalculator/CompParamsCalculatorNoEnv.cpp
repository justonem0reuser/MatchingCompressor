#include "CompParamsCalculatorNoEnv.h"
#include "QuantilesCalculator.h"
#include "../Data/Messages.h"
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
    lsfitfit(state, calculateFunctional, calculateGradient);
    lsfitresults(state, c, rep);

    if (rep.terminationtype < 0)
        throw std::exception(cannotCalculateErrStr.getCharPointer());
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

    double func = c[0];
    if (gradPtr != nullptr)
        (*gradPtr)[0] = 1.0;

    const int cLength = c.length();
    const int size = (cLength - 1) / 3;
    const double xDb = juce::Decibels::gainToDecibels(x[0], minusInfinityDb);

    int index = -1;
    for (int i = 0; i < size; i++)
    {
        auto i3 = 3 * i;
        auto curThreshold = c[i3 + 1];
        auto curKnee =
            c[i3 + 3] >= DynamicShaper<float>::minKneeWidth ?
            c[i3 + 3] : 0.0;

        int mask = (int)(xDb > curThreshold - 0.5 * curKnee);
        index += mask;
        if (i > 0)
            func += mask * (curThreshold - c[i3 - 2]) * (c[i3 - 1] - 1);
        if (gradPtr != nullptr)
        {
            (*gradPtr)[i3 + 1] = (*gradPtr)[i3 + 2] = (*gradPtr)[i3 + 3] = 0.0;
            if (i > 0 && mask != 0)
            {
                auto prevRatioInvM1 = c[i3 - 1] - 1;
                (*gradPtr)[i3 + 1] += prevRatioInvM1;
                (*gradPtr)[i3 - 2] -= prevRatioInvM1;
                (*gradPtr)[i3 - 1] += curThreshold - c[i3 - 2];
            }
        }
    }

    if (index == -1)
    {
        func += xDb;
        dbToGain(cLength, func, gradPtr);
        return func;
    }

    auto i3 = 3 * index;
    auto prevRatioInv = index == 0 ? 1.0 : c[i3 - 1];
    auto curThreshold = c[i3 + 1];
    auto curRatioInv = c[i3 + 2];
    auto curKneeWidth =
        c[i3 + 3] >= DynamicShaper<float>::minKneeWidth ?
        c[i3 + 3] : 0.0;
    auto curLeftBound = curThreshold - 0.5 * curKneeWidth;

    if (xDb >= curThreshold + 0.5 * curKneeWidth)
    {
        func += curThreshold + (xDb - curThreshold) * curRatioInv;
        if (gradPtr != nullptr)
        {
            (*gradPtr)[i3 + 1] += 1 - curRatioInv;
            (*gradPtr)[i3 + 2] += xDb - curThreshold;
        }
    }
    else
    {
        auto aQuadCoeff = 0.5 * (curRatioInv - prevRatioInv) / curKneeWidth;
        auto bQuadCoeff = prevRatioInv - 2.0 * aQuadCoeff * curLeftBound;
        auto cQuadCoeff = curThreshold -
            0.5 * prevRatioInv * curKneeWidth -
            curLeftBound * (curLeftBound * aQuadCoeff + bQuadCoeff);

        func += xDb * (xDb * aQuadCoeff + bQuadCoeff) + cQuadCoeff;

        if (gradPtr != nullptr)
        {
            auto dAdT = 0.0;
            auto dAdRInv = 0.5 / curKneeWidth;
            auto dAdKW = (prevRatioInv - curRatioInv) * dAdRInv / curKneeWidth;

            auto dBdT = -2.0 * aQuadCoeff;
            auto dBdRInv = -2.0 * curLeftBound * dAdRInv;
            auto dBdKW = aQuadCoeff - 2.0 * curLeftBound * dAdKW;

            auto dCdT = 1.0 - bQuadCoeff -
                curLeftBound * (2.0 * aQuadCoeff + curLeftBound * dAdT + dBdT);
            auto dCdRInv = -curLeftBound * (curLeftBound * dAdRInv + dBdRInv);
            auto dCdKW = 0.5 * (bQuadCoeff - prevRatioInv) +
                curLeftBound * (aQuadCoeff - curLeftBound * dAdKW - dBdKW);

            (*gradPtr)[i3 + 1] += xDb * (xDb * dAdT + dBdT) + dCdT;
            (*gradPtr)[i3 + 2] += xDb * (xDb * dAdRInv + dBdRInv) + dCdRInv;
            (*gradPtr)[i3 + 3] += xDb * (xDb * dAdKW + dBdKW) + dCdKW;

            if (index > 0)
            {
                auto dAdPrevRInv = -dAdRInv;
                auto dBdPrevRInv = 1 - 2.0 * curLeftBound * dAdPrevRInv;
                auto dCdPrevRInv = -0.5 * curKneeWidth -
                    curLeftBound * (curLeftBound * dAdPrevRInv + dBdPrevRInv);
                (*gradPtr)[i3 - 1] += xDb * (xDb * dAdPrevRInv + dBdPrevRInv) + dCdPrevRInv;
            }
        }
    }

    dbToGain(cLength, func, gradPtr);
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
