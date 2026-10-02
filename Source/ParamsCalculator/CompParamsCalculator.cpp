#include "CompParamsCalculator.h"
#include "GranularityCalculator.h"
#include "QuantilesCalculator.h"
#include "FuncAndGradCalculator.h"
#include "../Data/Messages.h"
#include "../Data/Ranges.h"
#include "../DSP/KWeightingFilter.h"
#include <algorithm>
#include <limits>

bool CompParamsCalculator::isKWeightingUsed(const juce::ValueTree& properties)
{
    return (int)properties.getProperty(setUseKWeightingId) == 2;
}

std::vector<std::vector<float>> CompParamsCalculator::applyKWeighting(
    const std::vector<std::vector<float>>& samples,
    double sampleRate)
{
    juce::ScopedNoDenormals noDenormals;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.numChannels = 1;
    spec.maximumBlockSize = 1;

    KWeightingFilter<float> filter;
    filter.prepare(spec);

    std::vector<std::vector<float>> weighted(samples.size());
    for (size_t i = 0; i < samples.size(); i++)
    {
        filter.reset();
        weighted[i].resize(samples[i].size());
        for (size_t j = 0; j < samples[i].size(); j++)
            weighted[i][j] = filter.processSample(samples[i][j]);
    }
    return weighted;
}

std::vector<bool> CompParamsCalculator::calculateGateMask(
    const std::vector<std::vector<float>>& samples,
    double sampleRate)
{
    const size_t numChannels = samples.size();
    const size_t numSamples = samples[0].size();
    const size_t blockLength = (size_t)juce::roundToInt(gateBlockLengthMs * 0.001 * sampleRate);
    const size_t blockStep = (size_t)juce::roundToInt(gateBlockStepMs * 0.001 * sampleRate);
    if (numSamples < blockLength)
        throw std::runtime_error(tooShortOrSilentExStr.toStdString());

    const size_t blocksNumber = (numSamples - blockLength + blockStep - 1) / blockStep + 1;
    std::vector<double> blockPowers(blocksNumber, 0.0);
    for (size_t i = 0; i < blocksNumber; i++)
    {
        const size_t end = std::min(i * blockStep + blockLength, numSamples);
        for (size_t j = 0; j < numChannels; j++)
            for (size_t k = i * blockStep; k < end; k++)
                blockPowers[i] += (double)samples[j][k] * samples[j][k];
        blockPowers[i] /= (double)(numChannels * blockLength);
    }

    std::vector<double> powers(blockPowers);
    const size_t percentileIndex = std::min(
        blocksNumber - 1,
        (size_t)std::ceil(gatePercentile * 0.01 * blocksNumber) - 1);
    std::nth_element(powers.begin(), powers.begin() + percentileIndex, powers.end());
    const double thresholdPower =
        powers[percentileIndex] * std::pow(10.0, 0.1 * gateRelativeThresholdDb);

    std::vector<bool> isSampleKept(numSamples, false);
    bool isAnyKept = false;
    for (size_t i = 0; i < blocksNumber; i++)
    {
        if (blockPowers[i] <= thresholdPower)
            continue;
        isAnyKept = true;
        const size_t end = std::min(i * blockStep + blockLength, numSamples);
        for (size_t j = i * blockStep; j < end; j++)
            isSampleKept[j] = true;
    }
    if (!isAnyKept)
        throw std::runtime_error(tooShortOrSilentExStr.toStdString());
    return isSampleKept;
}

std::vector<std::vector<float>> CompParamsCalculator::removeGatedSamples(
    const std::vector<std::vector<float>>& samples,
    const std::vector<bool>& isSampleKept,
    size_t keptSamplesNumber)
{
    std::vector<std::vector<float>> kept(samples.size());
    for (auto& ch : kept)
        ch.reserve(keptSamplesNumber);
    for (size_t i = 0; i < isSampleKept.size(); i++)
        if (isSampleKept[i])
            for (size_t j = 0; j < samples.size(); j++)
                kept[j].push_back(samples[j][i]);
    return kept;
}

void CompParamsCalculator::setKneeConstraints(
    alglib::lsfitstate& state,
    int kneesNumber,
    bool isWidthVariable)
{
    if (kneesNumber < 2)
        return;

    const int stride = getStride(isWidthVariable);
    const int rowsCount = kneesNumber - 1;
    const int cLength = getVectorLength(kneesNumber, stride);

    alglib::real_2d_array constraints;
    alglib::integer_1d_array types;
    constraints.setlength(rowsCount, cLength + 1);
    types.setlength(rowsCount);

    for (int i = 0; i < rowsCount; i++)
    {
        for (int j = 0; j <= cLength; j++)
            constraints[i][j] = 0.0;

        if (isWidthVariable)
        {
            constraints[i][getThresholdIndex(i, stride)] = -2.0;
            constraints[i][getThresholdIndex(i + 1, stride)] = 2.0;
            constraints[i][getKneeWidthIndex(i)] = -1.0;
            constraints[i][getKneeWidthIndex(i + 1)] = -1.0;
        }
        else
        {
            constraints[i][getThresholdIndex(i, stride)] = -1.0;
            constraints[i][getThresholdIndex(i + 1, stride)] = 1.0;
        }
        types[i] = 1; // >=
    }

    lsfitsetlc(state, constraints, types, rowsCount);
}

void CompParamsCalculator::enforceKneeGaps(
    alglib::real_1d_array& c,
    int kneesNumber,
    bool isWidthVariable)
{
    const int stride = getStride(isWidthVariable);
    for (int i = 1; i < kneesNumber; i++)
    {
        const int prevTInd = getThresholdIndex(i - 1, stride);
        const int curTInd = getThresholdIndex(i, stride);
        const double prevWidth = isWidthVariable ? c[getKneeWidthIndex(i - 1)] : 0.0;
        const double curWidth = isWidthVariable ? c[getKneeWidthIndex(i)] : 0.0;

        double violation =
            0.5 * (prevWidth + curWidth) - (c[curTInd] - c[prevTInd]);
        if (violation <= 0.0)
            continue;

        const double widthSum = prevWidth + curWidth;
        const double narrowing = std::min(2.0 * violation, widthSum);
        if (narrowing > 0.0)
        {
            const double scale = 1.0 - narrowing / widthSum;
            c[getKneeWidthIndex(i - 1)] = prevWidth * scale;
            c[getKneeWidthIndex(i)] = curWidth * scale;
            violation -= 0.5 * narrowing;
        }
        // Nothing left to narrow: the later threshold takes the rest.
        if (violation > 0.0)
            c[curTInd] += violation;
    }
}

void CompParamsCalculator::insertKneeWidths(
    alglib::real_1d_array& c,
    int kneesNumber,
    const double* widths)
{
    alglib::real_1d_array wide;
    wide.setlength(getVectorLength(kneesNumber, paramsPerKnee));
    wide[0] = c[0];
    for (int i = 0; i < kneesNumber; i++)
    {
        wide[getThresholdIndex(i, paramsPerKnee)] =
            c[getThresholdIndex(i, paramsPerKneeDerivedWidth)];
        wide[getRatioInverseIndex(i, paramsPerKnee)] =
            c[getRatioInverseIndex(i, paramsPerKneeDerivedWidth)];
        wide[getKneeWidthIndex(i)] = widths[i];
    }
    c = wide;
}

void CompParamsCalculator::calculateKneeWidths(
    const double* c,
    int kneesNumber,
    const double* nominalWidths,
    double* widths,
    double* dWidthDThreshold)
{
    for (int i = 0; i < kneesNumber; i++)
    {
        const double threshold = c[getThresholdIndex(i, paramsPerKneeDerivedWidth)];
        double width = nominalWidths[i];
        double derivative = 0.0;

        if (i > 0)
        {
            const double limit =
                2.0 * (threshold - c[getThresholdIndex(i - 1, paramsPerKneeDerivedWidth)])
                - nominalWidths[i - 1];
            if (limit < width)
            {
                width = limit;
                derivative = 2.0;
            }
        }
        if (i < kneesNumber - 1)
        {
            const double limit =
                2.0 * (c[getThresholdIndex(i + 1, paramsPerKneeDerivedWidth)] - threshold)
                - nominalWidths[i + 1];
            if (limit < width)
            {
                width = limit;
                derivative = -2.0;
            }
        }
        if (width < 0.0) // thresholds are too close for any width, or out of order
        {
            width = 0.0;
            derivative = 0.0;
        }

        widths[i] = width;
        if (dWidthDThreshold != nullptr)
            dWidthDThreshold[i] = derivative;
    }
}

void CompParamsCalculator::setInitGuessAndBounds(
    int kneesNumber,
    alglib::real_1d_array& c,
    alglib::real_1d_array& bndl,
    alglib::real_1d_array& bndu,
    bool isWidthVariable,
    float maxAmp)
{
    // c: Gain, [Threshold, 1/Ratio (, Knee weight)] * kneesNumber

    const int stride = getStride(isWidthVariable);
    const double thrOffsetDb = getThresholdOffsetDb(maxAmp);
    int cLength = getVectorLength(kneesNumber, stride);
    c.setlength(cLength);
    bndl.setlength(cLength);
    bndu.setlength(cLength);

    bndl[0] = gainRange.start;
    bndu[0] = gainRange.end;
    c[0] = 0.;

    for (int i = 0; i < kneesNumber; i++)
    {
        bndl[getThresholdIndex(i, stride)] = thresholdRange.start - thrOffsetDb;
        bndu[getThresholdIndex(i, stride)] = thresholdRange.end - thrOffsetDb;
        c[getThresholdIndex(i, stride)] = thresholdRange.start - thrOffsetDb +
            (thresholdRange.end - thresholdRange.start) *
            (i + 1) / (kneesNumber + 1);
        bndl[getRatioInverseIndex(i, stride)] = ratioInverseRange.start;
        bndu[getRatioInverseIndex(i, stride)] = ratioInverseRange.end;
        c[getRatioInverseIndex(i, stride)] = 1.;
        if (!isWidthVariable)
            continue;
        bndl[getKneeWidthIndex(i)] = kneeWidthRange.start;
        bndu[getKneeWidthIndex(i)] = kneeWidthRange.end;
        c[getKneeWidthIndex(i)] = 0.5 * (kneeWidthRange.start + kneeWidthRange.end);
    }
}

std::vector<float> CompParamsCalculator::resArrayToVector(const alglib::real_1d_array& c)
{
    const double* data = c.getcontent();
    return { data, data + c.length() };
}

float CompParamsCalculator::findMaxAmp(const std::vector<std::vector<float>>& samples)
{
    float maxAmp = 0.f;
    for (auto& ch : samples)
        for (float sample : ch)
            maxAmp = std::max(maxAmp, std::fabs(sample));
    return maxAmp;
}

void CompParamsCalculator::scaleSamples(
    const std::vector<std::vector<float>>& samples,
    std::vector<std::vector<float>>& scaled,
    float scale)
{
    scaled = samples;
    for (auto& ch : scaled)
        for (auto& sample : ch)
            sample *= scale;
}

float CompParamsCalculator::fitMismatch(double rmsError, const std::vector<float>& target)
{
    double sumSq = 0.0;
    for (float t : target)
        sumSq += (double)t * t;
    const double rmsTarget = std::sqrt(sumSq / std::max<size_t>(1, target.size()));
    if (rmsTarget <= 0.0)
        return std::numeric_limits<float>::infinity();
    return (float)(rmsError / rmsTarget);
}

void CompParamsCalculator::denormalize(std::vector<float>& result, float maxAmp)
{
    const float thresholdOffsetDb = getThresholdOffsetDb(maxAmp);
    if (thresholdOffsetDb == 0.f)
        return;
    const int kneesNumber = ((int)result.size() - 1) / 3;
    for (int k = 0; k < kneesNumber; ++k)
        result[1 + 3 * k] += thresholdOffsetDb;
}

void CompParamsCalculator::updateKneeWidths(const alglib::real_1d_array& c)
{
    if (nominalKneeWidths.empty())
        return;
    calculateKneeWidths(c.getcontent(), kneesNumber, nominalKneeWidths.data(),
        kneeWidths.data(), dKneeWidthDThreshold.data());
}

std::vector<float>& CompParamsCalculator::getY(const alglib::real_1d_array& c)
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

CompParamsCalculator::FunctionAndJacobian& CompParamsCalculator::getYAndJ(
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

std::vector<float> CompParamsCalculator::calculateCompressorParameters(
    std::vector<std::vector<float>>& refSamples,
    std::vector<std::vector<float>>& destSamples,
    double destSampleRate,
    juce::ValueTree& properties,
    const std::vector<std::vector<float>>* destDetectorSamples,
    const std::vector<bool>* isDestSampleKept)
{
    prepare(refSamples, destSamples, destSampleRate, properties, destDetectorSamples, isDestSampleKept);
    updateBallistics(
        properties.getProperty(setAttackId),
        properties.getProperty(setReleaseId),
        properties.getProperty(setHpfFrequencyId));
    return solve();
}

void CompParamsCalculator::updateEnvSettings(
    int balFilterTypeInt,
    int channelAggregationTypeInt,
    int useHpfInt)
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
    this->isHpfEnabled = useHpfInt == 2;
}

void CompParamsCalculator::configure(const juce::ValueTree& properties)
{
    int kneeTypeInt = properties.getProperty(setKneeTypeId);
    updateEnvSettings(
        properties.getProperty(setBalFilterTypeId),
        properties.getProperty(setChannelAggregationTypeId),
        properties.getProperty(setUseHpfId));
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

void CompParamsCalculator::setDestMask(const std::vector<bool>* isDestSampleKept)
{
    if (isDestSampleKept != nullptr)
    {
        jassert(isDestSampleKept->size() == destSamples[0].size());
        this->isDestSampleKept = *isDestSampleKept;
        keptDestSamplesNumber = (size_t)std::count(
            isDestSampleKept->begin(), isDestSampleKept->end(), true);
    }
    else
    {
        this->isDestSampleKept.clear();
        keptDestSamplesNumber = destSamples[0].size();
    }
}

void CompParamsCalculator::prepareForFixation(
    std::vector<std::vector<float>>& destSamples,
    double destSampleRate,
    juce::ValueTree& properties,
    const std::vector<std::vector<float>>* destDetectorSamples,
    const std::vector<bool>* isDestSampleKept)
{
    configure(properties);
    this->sampleRate = destSampleRate;
    maxAmp = findMaxAmp(destSamples);
    if (destDetectorSamples != nullptr)
        maxAmp = std::max(maxAmp, findMaxAmp(*destDetectorSamples));
    scaleSamples(destSamples, this->destSamples, getScale(maxAmp));
    if (destDetectorSamples != nullptr)
        scaleSamples(*destDetectorSamples, this->destDetectorSamples, getScale(maxAmp));
    else
        this->destDetectorSamples.clear();
    setDestMask(isDestSampleKept);
    spec.maximumBlockSize = 1000; // will not be used
}

void CompParamsCalculator::prepare(
    std::vector<std::vector<float>>& refSamples,
    std::vector<std::vector<float>>& destSamples,
    double destSampleRate,
    juce::ValueTree& properties,
    const std::vector<std::vector<float>>* destDetectorSamples,
    const std::vector<bool>* isDestSampleKept)
{
    this->sampleRate = destSampleRate;

    maxAmp = std::max(findMaxAmp(refSamples), findMaxAmp(destSamples));
    if (destDetectorSamples != nullptr)
        maxAmp = std::max(maxAmp, findMaxAmp(*destDetectorSamples));
    scaleSamples(destSamples, this->destSamples, getScale(maxAmp));
    if (destDetectorSamples != nullptr)
        scaleSamples(*destDetectorSamples, this->destDetectorSamples, getScale(maxAmp));
    else
        this->destDetectorSamples.clear();
    setDestMask(isDestSampleKept);

    spec.maximumBlockSize = 1000; // will not be used

    prepareStructure(refSamples, properties);
}

void CompParamsCalculator::prepareStructure(
    std::vector<std::vector<float>>& refSamples,
    const juce::ValueTree& properties)
{
    configure(properties);

    const int allRefSamplesNumber = refSamples.size() * refSamples[0].size();

    const float scale = getScale(maxAmp);
    referenceQuantiles.build(
        refSamples,
        gainRegionsNumber,
        quantileRegionsNumber,
        allRefSamplesNumber,
        scale);
    quantileRegionsNumber = (int)referenceQuantiles.get().size();
    if (kneeType == KneeType::soft)
        referenceQuantilesSoft.build(
            refSamples,
            gainRegionsNumberSoft,
            quantileRegionsNumberSoft,
            allRefSamplesNumber,
            scale);
}

void CompParamsCalculator::updateBallistics(
    float attackMs,
    float releaseMs,
    float hpfFrequency,
    bool isSoftNeeded)
{
    calculatedFunctions.clear();
    calculateEnvelopeStatistics(
        destSamples,
        destDetectorSamples.empty() ? destSamples : destDetectorSamples,
        sampleRate,
        attackMs,
        releaseMs,
        hpfFrequency,
        isSoftNeeded);
}

std::vector<float> CompParamsCalculator::solve()
{
    return solve(referenceQuantiles.get(), referenceQuantilesSoft.get());
}

std::vector<float> CompParamsCalculator::solve(
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

    setInitGuessAndBounds(kneesNumber, c, bndl, bndu, false, maxAmp);
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
            const double thrOffsetDb = getThresholdOffsetDb(maxAmp);
            bndl[0] = gainRange.start;
            bndu[0] = gainRange.end;
            s[0] = 1.;
            for (int i = 0; i < kneesNumber; i++)
            {
                bndl[getThresholdIndex(i, paramsPerKnee)] = thresholdRange.start - thrOffsetDb;
                bndu[getThresholdIndex(i, paramsPerKnee)] = thresholdRange.end - thrOffsetDb;
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

float CompParamsCalculator::fitMismatchAt(
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

void CompParamsCalculator::paramsToC(
    const std::vector<float>& params,
    alglib::real_1d_array& c,
    bool isWidthVariable)
{
    const int stride = getStride(isWidthVariable);
    jassert(c.length() == getVectorLength(kneesNumber, stride));
    jassert((int)params.size() >= getVectorLength(kneesNumber, paramsPerKnee));
    const double thrOffsetDb = getThresholdOffsetDb(maxAmp);
    c[0] = params[0];
    for (int k = 0; k < kneesNumber; k++)
    {
        c[getThresholdIndex(k, stride)] = params[1 + 3 * k] - thrOffsetDb;
        c[getRatioInverseIndex(k, stride)] = params[2 + 3 * k];
        if (isWidthVariable)
            c[getKneeWidthIndex(k)] = params[3 + 3 * k];
    }
}

std::vector<float> CompParamsCalculator::calculateQuantilesFor(const std::vector<float>& params)
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

void CompParamsCalculator::captureNominalKneeWidths(const std::vector<float>& params)
{
    jassert((int)params.size() >= 3 * kneesNumber + 1);
    fixationNominalWidths.resize(kneesNumber);
    for (int i = 0; i < kneesNumber; i++)
        fixationNominalWidths[i] = params[3 + 3 * i];
}

float CompParamsCalculator::scoreAgainstReference(const std::vector<float>& params)
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

void CompParamsCalculator::calculateFunctional(
    const alglib::real_1d_array& c,
    const alglib::real_1d_array& x,
    double& func,
    void* ptr)
{
    int index = (int)x[0];
    auto* calculator = (CompParamsCalculator*)ptr;
    auto& q = calculator->getY(c);
    auto coeff = juce::Decibels::decibelsToGain(c[0]);
    func = q[index] * coeff;
}

void CompParamsCalculator::calculateGradient(
    const alglib::real_1d_array& c,
    const alglib::real_1d_array& x,
    double& func,
    alglib::real_1d_array& grad,
    void* ptr)
{
    int index = (int)x[0];
    auto* calculator = (CompParamsCalculator*)ptr;
    auto& fj = calculator->getYAndJ(c);
    const double g = juce::Decibels::decibelsToGain(c[0]);
    const double lnCoeff = 0.05 * std::log(10.0);

    func = fj.q[index] * g;
    grad[0] = func * lnCoeff;
    const int cLength = c.length();
    for (int i = 1; i < cLength; i++)
        grad[i] = g * fj.jac[i][index];
}

std::vector<float> CompParamsCalculator::calculateFunction(
    std::vector<std::vector<float>>& samples,
    const alglib::real_1d_array& parameters,
    std::vector<std::vector<double>>* jacobian)
{
    auto samplesCount = samples.size() * keptDestSamplesNumber;
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

void CompParamsCalculator::calculateEnvelopeStatistics(
    std::vector<std::vector<float>>& samples,
    const std::vector<std::vector<float>>& detectorSamples,
    double sampleRate,
    float attackMs,
    float releaseMs,
    float hpfFrequency,
    bool isSoftNeeded)
{
    const bool isSoftBuilt = isSoftNeeded && kneeType == KneeType::soft;
    histogram.prepare(gainRegionsNumber);
    if (isSoftBuilt)
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
        isHpfEnabled,
        hpfFrequency,
        attackMs,
        releaseMs,
        balFilterType,
        channelAggregationType);
    dynamicProcessor.prepare(spec);

    const bool isAllKept = isDestSampleKept.empty();
    if (numChannels == 1)
    {
        for (size_t i = 0; i < numSamples; i++)
        {
            float sAbs = std::fabs(samples[0][i]);
            float env = dynamicProcessor.calculateEnv(0, detectorSamples[0][i]);
            if (isAllKept || isDestSampleKept[i])
            {
                histogram.add(sAbs, env);
                if (isSoftBuilt)
                    histogramSoft.add(sAbs, env);
            }
        }
    }
    else
    {
        for (size_t i = 0; i < numSamples; i++)
        {
            float sAbs0 = std::fabs(samples[0][i]);
            float sAbs1 = std::fabs(samples[1][i]);
            float out0, out1;
            dynamicProcessor.calculateStereoEnv(
                detectorSamples[0][i], detectorSamples[1][i], out0, out1);
            if (isAllKept || isDestSampleKept[i])
            {
                histogram.add(sAbs0, out0);
                histogram.add(sAbs1, out1);
                if (isSoftBuilt)
                {
                    histogramSoft.add(sAbs0, out0);
                    histogramSoft.add(sAbs1, out1);
                }
            }
        }
    }
    dynamicProcessor.reset();

    activeHistogram = &histogram;
}

std::vector<double> CompParamsCalculator::calculateYDensity(
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
