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
    
    referenceQuantiles.build(
        refNormalized, 
        gainRegionsNumber, 
        quantileRegionsNumber,
        allRefSamplesNumber);
    quantileRegionsNumber = (int)referenceQuantiles.get().size();
    if (kneeType == KneeType::soft)
        referenceQuantilesSoft.build(
            refNormalized, 
            gainRegionsNumberSoft, 
            quantileRegionsNumberSoft, 
            allRefSamplesNumber);
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
    return solve(referenceQuantiles.get(), referenceQuantilesSoft.get());
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

    setInitGuessAndBounds(kneesNumber, c, bndl, bndu, false);
    if (warmStart != nullptr)
    {
        jassert((int)warmStart->size() == getVectorLength(kneesNumber, paramsPerKnee));
        paramsToC(*warmStart, c, false);
        jassert((int)fixationNominalWidths.size() == kneesNumber);
        nominalKneeWidths = fixationNominalWidths;
        nominalKneeWidths.resize(kneesNumber, 0.0);
    }
    else
        nominalKneeWidths.assign(
            kneesNumber,
            kneeType == KneeType::soft ? 0.5 * (kneeWidthRange.start + kneeWidthRange.end) : 0.0);

    kneeWidths.assign(kneesNumber, 0.0);
    dKneeWidthDThreshold.assign(kneesNumber, 0.0);
    activeHistogram = &histogram;
    quantileRegionsNumber = (int)target.size();
    x.setlength(quantileRegionsNumber, 1);
    y.setlength(quantileRegionsNumber);
    s.setlength(getVectorLength(kneesNumber, paramsPerKneeDerivedWidth));

    for (int i = 0; i < quantileRegionsNumber; i++)
    {
        x[i][0] = i;// currentBinCenter;
        y[i] = target[i];
    }

    for (int i = 0; i < s.length(); i++)
        s[i] = 1.;

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

        calculateKneeWidths(c.getcontent(), kneesNumber, nominalKneeWidths.data(),
            kneeWidths.data());
        insertKneeWidths(c, kneesNumber, kneeWidths.data());
        nominalKneeWidths.clear();
        kneeWidths.clear();
        dKneeWidthDThreshold.clear();

        if (kneeType == KneeType::soft && warmStart == nullptr)
        {
            calculatedFunctions.clear();
            activeHistogram = &histogramSoft;
            quantileRegionsNumber = (int)targetSoft.size();
            x.setlength(quantileRegionsNumber, 1);
            y.setlength(quantileRegionsNumber);
            for (int i = 0; i < quantileRegionsNumber; i++)
            {
                x[i][0] = i;// currentBinCenter;
                y[i] = targetSoft[i];
            }

            bndl.setlength(c.length());
            bndu.setlength(c.length());
            s.setlength(c.length());
            bndl[0] = gainRange.start;
            bndu[0] = gainRange.end;
            s[0] = 1.;
            for (int i = 0; i < kneesNumber; i++)
            {
                bndl[getThresholdIndex(i, paramsPerKnee)] = thresholdRange.start;
                bndu[getThresholdIndex(i, paramsPerKnee)] = thresholdRange.end;
                bndl[getRatioInverseIndex(i, paramsPerKnee)] = ratioInverseRange.start;
                bndu[getRatioInverseIndex(i, paramsPerKnee)] = ratioInverseRange.end;
                bndl[getKneeWidthIndex(i)] = kneeWidthRange.start;
                bndu[getKneeWidthIndex(i)] = kneeWidthRange.end;
                s[getThresholdIndex(i, paramsPerKnee)] = 1.;
                s[getRatioInverseIndex(i, paramsPerKnee)] = 1.;
                s[getKneeWidthIndex(i)] = 100.;
            }

            lsfitcreatefg(x, y, c, true, state);
            lsfitsetcond(state, epsx, maxits);
            lsfitsetscale(state, s); 
            lsfitsetbc(state, bndl, bndu);
            // The widths are variables again.
            setKneeConstraints(state, kneesNumber, true);
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
    bool isWidthVariable)
{
    const int stride = getStride(isWidthVariable);
    jassert(c.length() == getVectorLength(kneesNumber, stride));
    jassert((int)params.size() >= getVectorLength(kneesNumber, paramsPerKnee));
    const double thrOffsetDb =
        (maxAmp <= 0.f || maxAmp == 1.f) ? 0.0 : 20.0 * std::log10(maxAmp);
    c[0] = params[0];
    for (int k = 0; k < kneesNumber; k++)
    {
        c[getThresholdIndex(k, stride)] = params[1 + 3 * k] - thrOffsetDb;
        c[getRatioInverseIndex(k, stride)] = params[2 + 3 * k];
        if (isWidthVariable)
            c[getKneeWidthIndex(k)] = params[3 + 3 * k];
    }
}

std::vector<float> CompParamsCalculatorEnv::calculateQuantilesFor(const std::vector<float>& params)
{
    alglib::real_1d_array c;
    c.setlength(getVectorLength(kneesNumber, paramsPerKneeDerivedWidth));
    paramsToC(params, c, false);

    activeHistogram = &histogram;
    calculatedFunctions.clear();

    jassert((int)fixationNominalWidths.size() == kneesNumber);
    nominalKneeWidths = fixationNominalWidths;
    nominalKneeWidths.resize(kneesNumber, 0.0);
    kneeWidths.assign(kneesNumber, 0.0);
    dKneeWidthDThreshold.assign(kneesNumber, 0.0);

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
    activeHistogram = soft ? &histogramSoft : &histogram;
    const auto& refTarget = soft ? referenceQuantilesSoft.get() : referenceQuantiles.get();
    jassert(!refTarget.empty());

    quantileRegionsNumber = (int)refTarget.size();
    calculatedFunctions.clear();

    nominalKneeWidths.clear();
    kneeWidths.clear();
    dKneeWidthDThreshold.clear();

    alglib::real_1d_array c;
    c.setlength(getVectorLength(kneesNumber, paramsPerKnee));
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
        const int binCount = activeHistogram->getSide();
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
    histogram.prepare(gainRegionsNumber);
    if (kneeType == KneeType::soft)
        histogramSoft.prepare(gainRegionsNumberSoft);

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
            histogram.add(sAbs, env);
            if (kneeType == KneeType::soft)
                histogramSoft.add(sAbs, env);
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
            histogram.add(sAbs0, out0);
            histogram.add(sAbs1, out1);
            if (kneeType == KneeType::soft)
            {
                histogramSoft.add(sAbs0, out0);
                histogramSoft.add(sAbs1, out1);
            }
        }
    }
    dynamicProcessor.reset();

    activeHistogram = &histogram;
}

std::vector<double> CompParamsCalculatorEnv::calculateYDensity(
    const alglib::real_1d_array& params,
    std::vector<std::vector<double>>* dBins)
{
    auto size = activeHistogram->getSide();
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
        const std::int32_t* row = activeHistogram->getRow(i);
        for (auto j = 0; j < size; j++)
        {
            auto weight = row[j];
            if (weight == 0)
                continue;

            double envDb = activeHistogram->getEnvDb(j);

            double yDb = FuncAndGradCalculator::calculateWithoutGain(
                envDb, 
                params.getcontent(), 
                getKneesNumber(n, getStride(widths == nullptr)),
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
