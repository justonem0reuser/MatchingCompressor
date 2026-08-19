#include <limits>
#include "CompParamsCalculatorEnv.h"
#include "GranularityCalculator.h"
#include "QuantilesCalculator.h"
#include "FuncAndGradCalculator.h"
#include "../Data/Messages.h"
#include "../Data/Ranges.h"
#include "interpolation.h"

void CompParamsCalculatorEnv::updateKneeWidths(const alglib::real_1d_array& c)
{
    if (nominalKneeWidths.empty())
        return;
    calculateKneeWidths(c.getcontent(), kneesNumber, nominalKneeWidths.data(),
        kneeWidths.data(), dKneeWidthDThreshold.data());
}

std::vector<float>& CompParamsCalculatorEnv::getY(const alglib::real_1d_array& c)
{
    //c : Gain, Threshold, 1/Ratio, Knee weight, attack, release
    auto it = calculatedFunctions.find(c);
    if (it == calculatedFunctions.end())
    {
        updateKneeWidths(c);
        FunctionAndJacobian fj;
        fj.q = calculateFunction(destSamples, c, nullptr);
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
        updateKneeWidths(c);
        FunctionAndJacobian fj;
        fj.q = calculateFunction(destSamples, c, &fj.jac); // q + jacobian
        fj.hasJacobian = true;
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
    prepare(refSamples, destSamples, destSampleRate, properties);
    updateBallistics(properties.getProperty(setAttackId), properties.getProperty(setReleaseId));
    return solve();
}

void CompParamsCalculatorEnv::updateEnvSettings(
    int balFilterTypeInt,
    int channelAggregationTypeInt)
{
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
}

void CompParamsCalculatorEnv::configure(const juce::ValueTree& properties)
{
    int kneeTypeInt = properties.getProperty(setKneeTypeId);
    updateEnvSettings(
        properties.getProperty(setBalFilterTypeId),
        properties.getProperty(setChannelAggregationTypeId));
    this->kneeType =
        kneeTypeInt == 1 ?
        KneeType::hard :
        KneeType::soft;

    this->kneesNumber = properties.getProperty(setKneesNumberId);

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
}

void CompParamsCalculatorEnv::prepareForFixation(
    std::vector<std::vector<float>>& destSamples,
    double destSampleRate,
    juce::ValueTree& properties)
{
    configure(properties);
    this->sampleRate = destSampleRate;
    std::vector<std::vector<float>> emptyRef, unusedRefNormalized;
    maxAmp = normalize(emptyRef, destSamples, unusedRefNormalized, this->destSamples);
    spec.maximumBlockSize = 1000; // will not be used
}

void CompParamsCalculatorEnv::prepare(
    std::vector<std::vector<float>>& refSamples,
    std::vector<std::vector<float>>& destSamples,
    double destSampleRate,
    juce::ValueTree& properties)
{
    configure(properties);
    this->sampleRate = destSampleRate;

    std::vector<std::vector<float>> refNormalized;
    maxAmp = normalize(refSamples, destSamples, refNormalized, this->destSamples);

    spec.maximumBlockSize = 1000; // will not be used

    const int allRefSamplesNumber = refSamples.size() * refSamples[0].size();
    
    auto refDensity = QuantilesCalculator::calculateDensityFunc(refNormalized, gainRegionsNumber);
    int nonEmptyBinsNumber = 0;
    for (double d : refDensity)
        if (d > 0.0)
            nonEmptyBinsNumber++;
    quantileRegionsNumber = GranularityCalculator::capQuantilesNumberByOccupancy(
        quantileRegionsNumber,
        nonEmptyBinsNumber);
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
        nonEmptyBinsNumber = 0;
        for (double d : refDensity)
            if (d > 0.0)
                nonEmptyBinsNumber++;
        quantileRegionsNumberSoft = GranularityCalculator::capQuantilesNumberByOccupancy(
            quantileRegionsNumberSoft,
            nonEmptyBinsNumber);
        referenceDensityFunctionSoft = QuantilesCalculator::density2Quantiles(
            refDensity,
            quantileRegionsNumberSoft,
            allRefSamplesNumber);
    }
}

void CompParamsCalculatorEnv::updateBallistics(float attackMs, float releaseMs)
{
    calculatedFunctions.clear();
    calculateEnvelopeStatistics(
        destSamples,
        sampleRate,
        attackMs,
        releaseMs);
}

std::vector<float> CompParamsCalculatorEnv::solve()
{
    return solve(referenceDensityFunction, referenceDensityFunctionSoft);
}

std::vector<float> CompParamsCalculatorEnv::solve(
    const std::vector<float>& target,
    const std::vector<float>& targetSoft,
    const std::vector<float>* warmStart)
{
    alglib::real_2d_array x;
    alglib::real_1d_array bndl, bndu, y, c, s;
    alglib::lsfitstate state;
    alglib::lsfitreport rep;

    nominalKneeWidths.clear();
    kneeWidths.clear();
    dKneeWidthDThreshold.clear();

    setInitGuessAndBounds(kneesNumber, kneeType, c, bndl, bndu);
    if (warmStart != nullptr)
    {
        jassert((int)warmStart->size() == c.length());
        paramsToC(*warmStart, c);
        nominalKneeWidths = fixationNominalWidths;
    }
    else
        nominalKneeWidths.assign(
            kneesNumber,
            kneeType == KneeType::soft ? 0.5 * (kneeWidthRange.start + kneeWidthRange.end) : 0.0);

    if (!nominalKneeWidths.empty())
    {
        kneeWidths.assign(kneesNumber, 0.0);
        dKneeWidthDThreshold.assign(kneesNumber, 0.0);
    }
    for (int i = 0; i < kneesNumber; i++)
        bndl[3 + 3 * i] = bndu[3 + 3 * i] = c[3 + 3 * i] = 0.0;
    activeEnvTable = &xEnvTable;
    activeEnvDbByCol = &envDbByCol;
    quantileRegionsNumber = (int)target.size();
    x.setlength(quantileRegionsNumber, 1);
    y.setlength(quantileRegionsNumber);
    s.setlength(3 * kneesNumber + 1);

    for (int i = 0; i < quantileRegionsNumber; i++)
    {
        x[i][0] = i;// currentBinCenter;
        y[i] = target[i];
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
        setKneeConstraints(state, kneesNumber, false);
        lsfitsetscale(state, s);
        lsfitfit(state, calculateFunctional, calculateGradient, nullptr, this);
        lsfitresults(state, c, rep);

        if (rep.terminationtype < 0)
            throw std::runtime_error(cannotCalculateErrStr.toStdString());

        enforceKneeGaps(c, kneesNumber, false);

        lastFitMismatch = fitMismatchAt(c, target);

        if (!nominalKneeWidths.empty())
        {
            calculateKneeWidths(c.getcontent(), kneesNumber, nominalKneeWidths.data(),
                kneeWidths.data());
            for (int i = 0; i < kneesNumber; i++)
                c[3 + 3 * i] = kneeWidths[i];
            nominalKneeWidths.clear();
            kneeWidths.clear();
            dKneeWidthDThreshold.clear();
        }

        if (kneeType == KneeType::soft && warmStart == nullptr)
        {
            calculatedFunctions.clear();
            activeEnvTable = &xEnvTableSoft;
            activeEnvDbByCol = &envDbByColSoft;
            quantileRegionsNumber = (int)targetSoft.size();
            x.setlength(quantileRegionsNumber, 1);
            y.setlength(quantileRegionsNumber);
            for (int i = 0; i < quantileRegionsNumber; i++)
            {
                x[i][0] = i;// currentBinCenter;
                y[i] = targetSoft[i];
            }

            for (int i = 0; i < kneesNumber; i++)
            {
                bndl[3 + 3 * i] = kneeWidthRange.start;
                bndu[3 + 3 * i] = kneeWidthRange.end;
            }

            lsfitcreatefg(x, y, c, true, state);
            lsfitsetcond(state, epsx, maxits);
            lsfitsetscale(state, s); 
            lsfitsetbc(state, bndl, bndu);
            setKneeConstraints(state, kneesNumber, true); // the widths are variables again
            lsfitfit(state, calculateFunctional, calculateGradient, nullptr, this);
            lsfitresults(state, c, rep);

            if (rep.terminationtype < 0)
                throw std::runtime_error(cannotCalculateErrStr.toStdString());

            enforceKneeGaps(c, kneesNumber, true);

            lastFitMismatch = fitMismatchAt(c, targetSoft);
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

float CompParamsCalculatorEnv::fitMismatchAt(
    const alglib::real_1d_array& c,
    const std::vector<float>& target)
{
    auto& q = getY(c);
    const double coeff = juce::Decibels::decibelsToGain(c[0]);
    const int n = (int)target.size();
    double sumSq = 0.0;
    for (int i = 0; i < n; i++)
    {
        double res = (double)q[i] * coeff - target[i];
        sumSq += res * res;
    }
    return fitMismatch(std::sqrt(sumSq / std::max(1, n)), target);
}

void CompParamsCalculatorEnv::paramsToC(
    const std::vector<float>& params,
    alglib::real_1d_array& c,
    bool keepKneeWidth)
{
    const double thrOffsetDb =
        (maxAmp <= 0.f || maxAmp == 1.f) ? 0.0 : 20.0 * std::log10(maxAmp);
    c[0] = params[0];
    for (int k = 0; k < kneesNumber; k++)
    {
        c[1 + 3 * k] = params[1 + 3 * k] - thrOffsetDb;
        c[2 + 3 * k] = params[2 + 3 * k];
        c[3 + 3 * k] = keepKneeWidth ? params[3 + 3 * k] : 0.0;
    }
}

std::vector<float> CompParamsCalculatorEnv::calculateQuantilesFor(const std::vector<float>& params)
{
    alglib::real_1d_array c;
    c.setlength(3 * kneesNumber + 1);
    paramsToC(params, c);

    activeEnvTable = &xEnvTable;
    activeEnvDbByCol = &envDbByCol;
    calculatedFunctions.clear();

    nominalKneeWidths = fixationNominalWidths;
    if (!nominalKneeWidths.empty())
    {
        kneeWidths.assign(kneesNumber, 0.0);
        dKneeWidthDThreshold.assign(kneesNumber, 0.0);
    }

    const auto& quantiles = getY(c);
    const double gain = juce::Decibels::decibelsToGain(c[0]);
    std::vector<float> res(quantiles.size());
    for (size_t i = 0; i < quantiles.size(); i++)
        res[i] = (float)((double)quantiles[i] * gain);

    nominalKneeWidths.clear();
    kneeWidths.clear();
    dKneeWidthDThreshold.clear();
    return res;
}

void CompParamsCalculatorEnv::captureNominalKneeWidths(const std::vector<float>& params)
{
    jassert((int)params.size() >= 3 * kneesNumber + 1);
    fixationNominalWidths.resize(kneesNumber);
    for (int i = 0; i < kneesNumber; i++)
        fixationNominalWidths[i] = params[3 + 3 * i];
}

float CompParamsCalculatorEnv::scoreAgainstReference(const std::vector<float>& params)
{
    const bool soft = (kneeType == KneeType::soft);
    activeEnvTable = soft ? &xEnvTableSoft : &xEnvTable;
    activeEnvDbByCol = soft ? &envDbByColSoft : &envDbByCol;
    const auto& refTarget = soft ? referenceDensityFunctionSoft : referenceDensityFunction;
    jassert(!refTarget.empty()); 

    quantileRegionsNumber = (int)refTarget.size();
    calculatedFunctions.clear();

    alglib::real_1d_array c;
    c.setlength(3 * kneesNumber + 1);
    paramsToC(params, c, true);
    const float mismatch = fitMismatchAt(c, refTarget);

    calculatedFunctions.clear();
    return mismatch;
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
    std::vector<std::vector<double>> dBins;
    std::vector<std::vector<double>>* dBinsPtr = nullptr;

    if (jacobian != nullptr)
    {
        const int parLength = parameters.length();
        const int binCount = (int)activeEnvDbByCol->size(); // == histogram columns
        dBinsPtr = &dBins;
        dBins.assign(parLength, std::vector<double>(binCount, 0.0));
        jacobian->assign(parLength, std::vector<double>(quantileRegionsNumber, 0.0));
    }

    auto yDensity = calculateYDensity(parameters, dBinsPtr);
    return QuantilesCalculator::density2Quantiles(
        yDensity, quantileRegionsNumber, samplesCount, dBinsPtr, jacobian);
}

void CompParamsCalculatorEnv::calculateEnvelopeStatistics(
    std::vector<std::vector<float>>& samples,
    double sampleRate,
    float attackMs,
    float releaseMs)
{
    xEnvTable.assign((size_t)(gainRegionsNumber * gainRegionsNumber), 0);

    const double delta = 1.0 / gainRegionsNumber;
    envDbByCol.resize(gainRegionsNumber);
    for (int j = 0; j < gainRegionsNumber; j++)
        envDbByCol[j] = juce::Decibels::gainToDecibels(
            (j + 0.5) * delta, DynamicShaper<double>::minusInfinityDb);

    if (kneeType == KneeType::soft)
    {
        xEnvTableSoft.assign((size_t)(gainRegionsNumberSoft * gainRegionsNumberSoft), 0);

        const double deltaSoft = 1.0 / gainRegionsNumberSoft;
        envDbByColSoft.resize(gainRegionsNumberSoft);
        for (int j = 0; j < gainRegionsNumberSoft; j++)
            envDbByColSoft[j] = juce::Decibels::gainToDecibels(
                (j + 0.5) * deltaSoft, DynamicShaper<double>::minusInfinityDb);
    }

    auto numChannels = samples.size();
    auto numSamples = samples[0].size();

    jassert((long long)numChannels * (long long)numSamples
        <= (long long)std::numeric_limits<std::int32_t>::max());

    spec.numChannels = numChannels;
    spec.sampleRate = sampleRate;
    jassert((long long)numChannels * (long long)numSamples
        <= (long long)std::numeric_limits<std::int32_t>::max());

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
            xEnvTable[i1 * gainRegionsNumber + i2]++;
            if (kneeType == KneeType::soft)
            {
                i1 = std::min((int)(sAbs * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                i2 = std::min((int)(env * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                xEnvTableSoft[i1 * gainRegionsNumberSoft + i2]++;
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
            xEnvTable[i1 * gainRegionsNumber + i2]++;
            i1 = std::min((int)(sAbs1 * gainRegionsNumber), gainRegionsNumber - 1);
            i2 = std::min((int)(out1 * gainRegionsNumber), gainRegionsNumber - 1);
            xEnvTable[i1 * gainRegionsNumber + i2]++;
            if (kneeType == KneeType::soft)
            {
                i1 = std::min((int)(sAbs0 * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                i2 = std::min((int)(out0 * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                xEnvTableSoft[i1 * gainRegionsNumberSoft + i2]++;
                i1 = std::min((int)(sAbs1 * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                i2 = std::min((int)(out1 * gainRegionsNumberSoft), gainRegionsNumberSoft - 1);
                xEnvTableSoft[i1 * gainRegionsNumberSoft + i2]++;
            }
        }
    }
    dynamicProcessor.reset();
    activeEnvTable = &xEnvTable;
    activeEnvDbByCol = &envDbByCol;
}

std::vector<double> CompParamsCalculatorEnv::calculateYDensity(
    const alglib::real_1d_array& params,
    std::vector<std::vector<double>>* dBins)
{
    auto size = (int)activeEnvDbByCol->size(); // one entry per histogram column
    float delta = 1.f / size;
    std::vector<double> res(size, 0.0);

    const double scaleCoeff = 0.05 * std::log(10.0);
    const double* widths = kneeWidths.empty() ? nullptr : kneeWidths.data();
    const double* dWidths = kneeWidths.empty() ? nullptr : dKneeWidthDThreshold.data();
    const int n = params.length();
    std::vector<double> grad(n);
    alglib::real_1d_array gradDb;
    if (dBins != nullptr)
        gradDb.setlength(n);

    for (auto i = 0; i < size; i++)
    {
        float x = (i + 0.5f) * delta; // recalculate each step to increase precision
        const std::int32_t* row = activeEnvTable->data() + i * size;
        for (auto j = 0; j < size; j++)
        {
            auto weight = row[j];
            if (weight == 0)
                continue;

            double envDb = (*activeEnvDbByCol)[j];

            double yDb = FuncAndGradCalculator::calculateWithoutGain(
                envDb, 
                params.getcontent(), 
                (n - 1) / 3,
                false, // true would require division by envDb
                dBins != nullptr ? gradDb.getcontent() : nullptr,
                widths,
                dWidths);
            double yAbs = x * juce::Decibels::decibelsToGain(yDb - envDb);

            if (dBins == nullptr)
                QuantilesCalculator::putToBins(yAbs, res, weight);
            else
            {
                double scale = yAbs * scaleCoeff;
                for (int p = 1; p < n; p++) // p = 0 is gain
                    grad[p] = scale * gradDb[p];
                QuantilesCalculator::putToBins(yAbs, res, weight, &grad, dBins);
            }
        }
    }
    return res;
}
