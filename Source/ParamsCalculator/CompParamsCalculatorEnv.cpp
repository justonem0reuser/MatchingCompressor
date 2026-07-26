#include "CompParamsCalculatorEnv.h"
#include "GranularityCalculator.h"
#include "QuantilesCalculator.h"
#include "FuncAndGradCalculator.h"
#include "../Data/Messages.h"
#include "../Data/Ranges.h"
#include "interpolation.h"

void CompParamsCalculatorEnv::addFine(
    const alglib::real_1d_array& c,
    FunctionAndJacobian& fj,
    bool withJacobian)
{
    const int cLength = c.length();
    const int qLength = (int)fj.q.size();

    alglib::real_1d_array fineGrad;
    if (withJacobian)
    {
        fineGrad.setlength(cLength);
        for (int p = 0; p < cLength; p++)
            fineGrad[p] = 0.0;
    }

    float fine = (float)calculateFine(c, withJacobian ? &fineGrad : nullptr);
    for (int i = 0; i < qLength; i++)
        fj.q[i] += fine;

    if (withJacobian)
        for (int i = 0; i < cLength; i++)
            if (fineGrad[i] != 0.0)
                for (int j = 0; j < qLength; j++)
                    fj.jac[i][j] += fineGrad[i];
}

std::vector<float>& CompParamsCalculatorEnv::getY(const alglib::real_1d_array& c)
{
    //c : Gain, Threshold, 1/Ratio, Knee weight, attack, release
    auto it = calculatedFunctions.find(c);
    if (it == calculatedFunctions.end())
    {
        FunctionAndJacobian fj;
        fj.q = calculateFunction(destSamples, c, nullptr);
        addFine(c, fj, false);
        it = calculatedFunctions.emplace(c, std::move(fj)).first;
    }
    return it->second.q;
}

CompParamsCalculatorEnv::FunctionAndJacobian& CompParamsCalculatorEnv::getYAndJ(
    const alglib::real_1d_array& c)
{
    //c : Gain, Threshold, 1/Ratio, Knee weight, attack, release
    auto it = calculatedFunctions.find(c);
    if (it == calculatedFunctions.end() || !it->second.hasJacobian)
    {
        FunctionAndJacobian fj;
        fj.q = calculateFunction(destSamples, c, &fj.jac); // q + jacobian
        fj.hasJacobian = true;
        addFine(c, fj, true);
        if (it == calculatedFunctions.end())
            it = calculatedFunctions.emplace(c, std::move(fj)).first;
        else
            it->second = std::move(fj);
    }
    return it->second;
}

std::vector<float> CompParamsCalculatorEnv::calculateCompressorParameters(
    std::vector<std::vector<float>>& refSamples, 
    std::vector<std::vector<float>>& destSamples, 
    double destSampleRate,
    juce::ValueTree& properties)
{
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

    auto gHard = GranularityCalculator::calculateTargetGranularity(
        kneesNumber, false);
    gainRegionsNumber = gHard.gainRegions;
    quantileRegionsNumber = gHard.quantileRegions;
    if (kneeType == KneeType::soft)
    {
        auto gSoft = GranularityCalculator::calculateTargetGranularity(
            kneesNumber, true);
        gainRegionsNumberSoft = gSoft.gainRegions;
        quantileRegionsNumberSoft = gSoft.quantileRegions;
    }
    else
    {
        gainRegionsNumberSoft = gainRegionsNumber;
        quantileRegionsNumberSoft = quantileRegionsNumber;
    }

    std::vector<std::vector<float>> refNormalized;
    const float maxAmp = normalize(refSamples, destSamples, refNormalized, this->destSamples);

    spec.maximumBlockSize = 1000; // will not be used

    calculatedFunctions.clear();
    calculateEnvelopeStatistics(
        this->destSamples, 
        destSampleRate, 
        attackMs, 
        releaseMs);

    const int allRefSamplesNumber = refSamples.size() * refSamples[0].size();
    std::vector<float> referenceDensityFunction, referenceDensityFunctionSoft;

    auto refDensity = QuantilesCalculator::calculateDensityFunc(
        refNormalized,
        gainRegionsNumber);
    int nonEmptyBeansNumber = 0;
    for (double d : refDensity)
        if (d > 0.0)
            nonEmptyBeansNumber++;
    quantileRegionsNumber = GranularityCalculator::capQuantilesNumberByOccupancy(
        quantileRegionsNumber,
        nonEmptyBeansNumber);
    referenceDensityFunction =
        QuantilesCalculator::density2Quantiles(
            refDensity,
            quantileRegionsNumber,
            allRefSamplesNumber);
    if (kneeType == KneeType::soft)
    {
        refDensity = QuantilesCalculator::calculateDensityFunc(
            refNormalized,
            gainRegionsNumberSoft);
        nonEmptyBeansNumber = 0;
        for (double d : refDensity)
            if (d > 0.0)
                nonEmptyBeansNumber++;
        quantileRegionsNumberSoft = GranularityCalculator::capQuantilesNumberByOccupancy(
            quantileRegionsNumberSoft,
            nonEmptyBeansNumber);
        referenceDensityFunctionSoft = QuantilesCalculator::density2Quantiles(
            refDensity,
            quantileRegionsNumberSoft,
            allRefSamplesNumber);
    }

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

    try
    {
        lsfitcreatefg(x, y, c, true, state);
        lsfitsetcond(state, epsx, maxits);
        lsfitsetbc(state, bndl, bndu);
        lsfitsetscale(state, s);
        lsfitfit(state, calculateFunctional, calculateGradient, nullptr, this);
        lsfitresults(state, c, rep);

        if (rep.terminationtype < 0)
            throw std::runtime_error(cannotCalculateErrStr.toStdString());

        if (kneeType == KneeType::soft)
        {
            calculatedFunctions.clear();
            activeEnvTable = &xEnvTableSoft;
            activeEnvDbByCol = &envDbByColSoft;
            gainRegionsNumber = gainRegionsNumberSoft;
            quantileRegionsNumber = quantileRegionsNumberSoft;
            x.setlength(quantileRegionsNumberSoft, 1);
            y.setlength(quantileRegionsNumberSoft);
            for (int i = 0; i < quantileRegionsNumberSoft; i++)
            {
                x[i][0] = i;// currentBeanCenter;
                y[i] = referenceDensityFunctionSoft[i];
            }

            for (int i = 0; i < kneesNumber; i++)
            {
                bndl[3 + 3 * i] = kneeWidthRange.start;
                bndu[3 + 3 * i] = kneeWidthRange.end;
                c[3 + 3 * i] = 0.5 * (kneeWidthRange.start + kneeWidthRange.end);
            }
        
            // check knees intersection
            for (int i = 1; i < kneesNumber; i++)
            {
                auto maxWidthsSum = 2.0 * (c[3 * i + 1] - c[3 * i - 2] - fineThreshold);
                auto widthsSum = c[3 * i] + c[3 * i + 3];
                if (widthsSum > maxWidthsSum)
                {
                    auto k = maxWidthsSum > 0.0 ? maxWidthsSum / widthsSum : 0.0;
                    c[3 * i] *= k;
                    c[3 * i + 3] *= k;
                }
            }

            lsfitcreatefg(x, y, c, true, state);
            lsfitsetcond(state, epsx, maxits);
            lsfitsetscale(state, s);
            lsfitsetbc(state, bndl, bndu);
            lsfitfit(state, calculateFunctional, calculateGradient, nullptr, this);
            lsfitresults(state, c, rep);

            if (rep.terminationtype < 0)
                throw std::runtime_error(cannotCalculateErrStr.toStdString());
        }

        auto result = resArrayToVector(c);
        denormalize(result, maxAmp);
        return result;
    }
    catch (const alglib::ap_error&)
    {
        throw std::runtime_error(cannotCalculateErrStr.toStdString());
    }
}

void CompParamsCalculatorEnv::calculateFunctional(
    const alglib::real_1d_array& c, 
    const alglib::real_1d_array& x, 
    double& func, 
    void* ptr)
{
    int index = (int)x[0];
    auto* calculator = (CompParamsCalculatorEnv*)ptr;
    auto& q = calculator->getY(c);
    auto coeff = juce::Decibels::decibelsToGain(c[0]);
    func = q[index] * coeff;
}

void CompParamsCalculatorEnv::calculateGradient(
    const alglib::real_1d_array& c,
    const alglib::real_1d_array& x,
    double& func,
    alglib::real_1d_array& grad,
    void* ptr)
{
    int index = (int)x[0];
    auto* calculator = (CompParamsCalculatorEnv*)ptr;
    auto& fj = calculator->getYAndJ(c);
    const double g = juce::Decibels::decibelsToGain(c[0]);
    const double lnCoeff = 0.05 * std::log(10.0);

    func = fj.q[index] * g;
    grad[0] = func * lnCoeff;
    const int cLength = c.length();
    for (int i = 1; i < cLength; i++)
        grad[i] = g * fj.jac[i][index];
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

std::vector<float> CompParamsCalculatorEnv::calculateFunction(
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
        const int beanCount = (int)activeEnvTable->cols();
        dBeansPtr = &dBeans;
        dBeans.assign(parLength, std::vector<double>(beanCount, 0.0));
        jacobian->assign(parLength, std::vector<double>(quantileRegionsNumber, 0.0));
    }

    auto yDensity = calculateYDensity(parameters, dBeansPtr);
    return QuantilesCalculator::density2Quantiles(
        yDensity, quantileRegionsNumber, samplesCount, dBeansPtr, jacobian);
}

void CompParamsCalculatorEnv::calculateEnvelopeStatistics(
    std::vector<std::vector<float>>& samples,
    double sampleRate,
    float attackMs,
    float releaseMs)
{
    const bool isSoftRequired =
        gainRegionsNumber != gainRegionsNumberSoft ||
        quantileRegionsNumber != quantileRegionsNumberSoft;

    xEnvTable.setlength(gainRegionsNumber, gainRegionsNumber);
    for (int i = 0; i < gainRegionsNumber; i++)
        for (int j = 0; j < gainRegionsNumber; j++)
            xEnvTable[i][j] = 0.;

    const double delta = 1.0 / gainRegionsNumber;
    envDbByCol.resize(gainRegionsNumber);
    for (int j = 0; j < gainRegionsNumber; j++)
        envDbByCol[j] = juce::Decibels::gainToDecibels(
            (j + 0.5) * delta, DynamicShaper<double>::minusInfinityDb);

    if (isSoftRequired)
    {
        xEnvTableSoft.setlength(gainRegionsNumberSoft, gainRegionsNumberSoft);
        for (int i = 0; i < gainRegionsNumberSoft; i++)
            for (int j = 0; j < gainRegionsNumberSoft; j++)
                xEnvTableSoft[i][j] = 0.;

        const double deltaSoft = 1.0 / gainRegionsNumberSoft;
        envDbByColSoft.resize(gainRegionsNumberSoft);
        for (int j = 0; j < gainRegionsNumberSoft; j++)
            envDbByColSoft[j] = juce::Decibels::gainToDecibels(
                (j + 0.5) * deltaSoft, DynamicShaper<double>::minusInfinityDb);
    }

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
            if (isSoftRequired)
            {
                i1 = std::min((int)(sAbs * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                i2 = std::min((int)(env * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                xEnvTableSoft[i1][i2]++;
            }
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
            if (isSoftRequired)
            {
                i1 = std::min((int)(sAbs0 * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                i2 = std::min((int)(out0 * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                xEnvTableSoft[i1][i2]++;
                i1 = std::min((int)(sAbs1 * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                i2 = std::min((int)(out1 * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                xEnvTableSoft[i1][i2]++;
            }
        }
    }
    dynamicProcessor.reset();
    activeEnvTable = &xEnvTable;
    activeEnvDbByCol = &envDbByCol;
}

std::vector<double> CompParamsCalculatorEnv::calculateYDensity(
    const alglib::real_1d_array& params,
    std::vector<std::vector<double>>* dBeans)
{
    auto size = (int)activeEnvTable->cols();
    float delta = 1.f / size;
    std::vector<double> res(size, 0.0);

    const double scaleCoeff = 0.05 * std::log(10.0);
    const int n = params.length();
    std::vector<double> grad(n);
    alglib::real_1d_array gradDb;
    if (dBeans != nullptr)
        gradDb.setlength(n);

    for (auto i = 0; i < size; i++)
    {
        float x = (i + 0.5f) * delta; // recalculate each step to increase precision
        for (auto j = 0; j < size; j++)
        {
            auto weight = (*activeEnvTable)[i][j];
            if (weight == 0)
                continue;

            double envDb = (*activeEnvDbByCol)[j];

            double yDb = FuncAndGradCalculator::calculateWithoutGain(
                envDb, params, false, dBeans != nullptr ? &gradDb : nullptr); // true would require division by envDb
            double yAbs = x * juce::Decibels::decibelsToGain(yDb - envDb);

            if (dBeans == nullptr)
                QuantilesCalculator::putToBeans(yAbs, res, weight);
            else
            {
                double scale = yAbs * scaleCoeff;
                for (int p = 1; p < n; p++) // p = 0 is gain
                    grad[p] = scale * gradDb[p];
                QuantilesCalculator::putToBeans(yAbs, res, weight, &grad, dBeans);
            }
        }
    }
    return res;
}
