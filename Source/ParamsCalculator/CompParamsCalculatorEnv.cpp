#include "CompParamsCalculatorEnv.h"
#include "QuantilesCalculator.h"
#include "../Data/Messages.h"
#include "../Data/Ranges.h"
#include "interpolation.h"

std::vector<float>& CompParamsCalculatorEnv::getY(const alglib::real_1d_array& c)
{
    //c : Gain, Threshold, 1/Ratio, Knee weight, attack, release
    if (calculatedFunctions.count(c) == 0)
    {
        calculatedFunctions[c] = calculateFunction(destSamples, c);
        double fine = calculateFine(c);
        if (fine > 0.)
            for (auto i = 0; i < calculatedFunctions[c].size(); i++)
                calculatedFunctions[c][i] += fine;
    }
    return calculatedFunctions[c];
}

std::vector<float> CompParamsCalculatorEnv::calculateCompressorParameters(
    std::vector<std::vector<float>>& refSamples, 
    double refSampleRate,
    std::vector<std::vector<float>>& destSamples, 
    double destSampleRate,
    juce::ValueTree& properties)
{
    int gainRegionsNumber = properties.getProperty(setGainRegionsNumberId);
    int quantileRegionsNumber = properties.getProperty(setQuantileRegionsNumberId);
    
    jassert(quantileRegionsNumber > 2 && gainRegionsNumber >= quantileRegionsNumber);

    this->quantileRegionsNumber = quantileRegionsNumber;
    this->gainRegionsNumber = gainRegionsNumber;

    int balFilterTypeInt = properties.getProperty(setBalFilterTypeId);
    int channelAggregationTypeInt = properties.getProperty(setChannelAggregationTypeId);
    int kneeTypeInt = properties.getProperty(setKneeTypeId);
    this->balFilterType =
        balFilterTypeInt == 1 ?
        EnvCalculationType::peak :
        EnvCalculationType::RMS;
    this->channelAggregationType =
        channelAggregationTypeInt == 1 ? 
        ChannelAggregationType::separate :
        channelAggregationTypeInt == 2 ? 
        ChannelAggregationType::max :
        ChannelAggregationType::mean;
    KneeType kneeType =  
        kneeTypeInt == 1 ?
        KneeType::hard :
        KneeType::soft;

    int kneesNumber = properties.getProperty(setKneesNumberId);
    float attackMs = properties.getProperty(setAttackId);
    float releaseMs = properties.getProperty(setReleaseId);

    this->destSamples = destSamples;

    spec.maximumBlockSize = 1000; // will not be used

    calculatedFunctions.clear();
    calculateEnvelopeStatistics(
        this->destSamples, 
        refSampleRate, 
        attackMs, 
        releaseMs);

    auto referenceDensityFunction = 
        QuantilesCalculator::calculateQuantiles(
            refSamples, 
            gainRegionsNumber, 
            quantileRegionsNumber);

    alglib::real_2d_array x;
    alglib::real_1d_array bndl, bndu, y, c, s;
    alglib::lsfitstate state;
    alglib::lsfitreport rep;

    setInitGuessAndBounds(kneesNumber, kneeType, c, bndl, bndu);
    x.setlength(quantileRegionsNumber, 1);
    y.setlength(quantileRegionsNumber);
    s.setlength(3 * kneesNumber + 1);

    for (int i = 0; i < quantileRegionsNumber; i++)
    {
        x[i][0] = i;// currentBeanCenter;
        y[i] = referenceDensityFunction[i];
    }

    s[0] = 1.;
    for (int i = 0; i < kneesNumber; i++)
    {
        s[1 + 3 * i] = s[2 + 3 * i] = 1.;
        s[3 + 3 * i] = 100.;
    }

    lsfitcreatef(x, y, c, diffstep, state);
    lsfitsetcond(state, epsx, maxits);
    lsfitsetbc(state, bndl, bndu);
    lsfitsetscale(state, s);
    lsfitfit(state, calculateFunctional, nullptr, this);
    lsfitresults(state, c, rep);

    if (rep.terminationtype < 0)
        throw std::runtime_error(cannotCalculateErrStr.toStdString());

    if (kneeType == KneeType::soft)
    {
        for (int i = 0; i < kneesNumber; i++)
        {
            bndl[3 + 3 * i] = kneeWidthRange.start;
            bndu[3 + 3 * i] = kneeWidthRange.end;
            c[3 + 3 * i] = 0.5f * (kneeWidthRange.start + kneeWidthRange.end);
        }
        lsfitsetbc(state, bndl, bndu);
        lsfitfit(state, calculateFunctional, nullptr, this);
        lsfitresults(state, c, rep);

        if (rep.terminationtype < 0)
            throw std::runtime_error(cannotCalculateErrStr.toStdString());
    }
    
    return resArrayToVector(c);
}

void CompParamsCalculatorEnv::calculateFunctional(
    const alglib::real_1d_array& c, 
    const alglib::real_1d_array& x, 
    double& func, 
    void* ptr)
{
    int index = (int)x[0];
    auto* calculator = (CompParamsCalculatorEnv*)ptr;
    auto& yVector = calculator->getY(c);
    auto coeff = juce::Decibels::decibelsToGain(c[0]);
    func = yVector[index] * coeff;
}

void CompParamsCalculatorEnv::setCompParameters(const alglib::real_1d_array& params)
{
    jassert(params.length() >= 4 && (params.length() - 1) % 3 == 0);
    int size = (params.length() - 1) / 3;
    DynamicShaper<float>::KneesArray newThresholdsDb, newRatios, newWidthsDb;
    for (int i = 0; i < size; i++)
    {
        newThresholdsDb[i] = params[1 + i * 3];
        newRatios[i] = 1.0 / params[2 + i * 3];
        newWidthsDb[i] = params[3 + i * 3];
    }
    dynamicProcessor.setCompParameters(
        newThresholdsDb, 
        newRatios, 
        newWidthsDb, 
        0., // gain will be taken into account in comp_func
        size);
}

