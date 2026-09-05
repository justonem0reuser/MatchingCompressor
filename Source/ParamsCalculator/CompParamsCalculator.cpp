#include "CompParamsCalculator.h"
#include "../Data/Ranges.h"
#include <algorithm>
#include <limits>

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