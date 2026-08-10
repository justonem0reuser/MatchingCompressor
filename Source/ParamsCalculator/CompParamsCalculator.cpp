#include "CompParamsCalculator.h"
#include "../Data/Ranges.h"

double CompParamsCalculator::calculateFine(
    const alglib::real_1d_array& c, 
    alglib::real_1d_array* gradPtr)
{
    int size = (c.length() - 1) / 3;
    if (size <= 1)
        return 0;
    double fine = 0;
    for (int i = 1; i < size; i++)
    {
        int prevKneeInd = 3 * i;
        int curKneeInd = prevKneeInd + 3;
        int prevTInd = prevKneeInd - 2;
        int curTInd = prevKneeInd + 1;

        auto curLeftBound = c[curTInd] - 0.5 * c[curKneeInd];
        auto prevRightBound = c[prevTInd] + 0.5 * c[prevKneeInd];

        auto fineDelta = curLeftBound - prevRightBound - fineThreshold;
        if (fineDelta < 0.0)
        {
            fine += fineCoeff * fineDelta * fineDelta;
            if (gradPtr != nullptr)
            {
                auto& grad = *gradPtr;
                double valueToAdd = fineCoeff * fineDelta;
                grad[prevTInd]    -= 2.0 * valueToAdd;
                grad[prevKneeInd] -= valueToAdd;
                grad[curTInd]     += 2.0 * valueToAdd;
                grad[curKneeInd]  -= valueToAdd;
            }
        }
    }
    return fine;
}

void CompParamsCalculator::setInitGuessAndBounds(
    int kneesNumber,
    KneeType kneeType,
    alglib::real_1d_array& c,
    alglib::real_1d_array& bndl, 
    alglib::real_1d_array& bndu)
{
    // c: Gain, [Threshold, 1/Ratio, Knee weight] * kneesNum

    int cLength = 3 * kneesNumber + 1;
    c.setlength(cLength);
    bndl.setlength(cLength);
    bndu.setlength(cLength);

    bndl[0] = gainRange.start;
    bndu[0] = gainRange.end;
    c[0] = 0.;

    for (int i = 0; i < kneesNumber; i++)
    {
        bndl[1 + 3 * i] = thresholdRange.start;
        bndu[1 + 3 * i] = thresholdRange.end;
        c[1 + 3 * i] = thresholdRange.start +
            (thresholdRange.end - thresholdRange.start) * 
            (i + 1) / (kneesNumber + 1);
        bndl[2 + 3 * i] = 1. / ratioRange.end;
        bndu[2 + 3 * i] = 2. - ratioRange.start;
        c[2 + 3 * i] = 1.;
        if (kneeType == KneeType::hard)
            bndl[3 + 3 * i] = bndu[3 + 3 * i] = c[3 + 3 * i] = 0;
        else
        {
            bndl[3 + 3 * i] = kneeWidthRange.start;
            bndu[3 + 3 * i] = kneeWidthRange.end;
            c[3 + 3 * i] = 0.5 * (kneeWidthRange.start + kneeWidthRange.end);
        }
    }
}

std::vector<float> CompParamsCalculator::resArrayToVector(alglib::real_1d_array& c)
{
    auto cLength = c.length();
    auto kneesNumber = (cLength - 1) / 3;
    std::vector<float> res(cLength);
    res[0] = c[0];
    for (auto i = 0; i < kneesNumber; i++)
    {
        res[3 * i + 1] = c[3 * i + 1];
        res[3 * i + 2] = 1. / c[3 * i + 2];
        res[3 * i + 3] = c[3 * i + 3];
    }
    return res;
}

float CompParamsCalculator::normalize(
    const std::vector<std::vector<float>>& refSamples,
    const std::vector<std::vector<float>>& destSamples,
    std::vector<std::vector<float>>& refNormalized,
    std::vector<std::vector<float>>& destNormalized)
{
    float maxAmp = 0.f;
    for (auto& ch : refSamples)
        for (float sample : ch)
            maxAmp = std::max(maxAmp, std::fabs(sample));
    for (auto& ch : destSamples)
        for (float sample : ch)
            maxAmp = std::max(maxAmp, std::fabs(sample));

    const float scale = maxAmp > 0.f ? 1.f / maxAmp : 1.f;
    refNormalized = refSamples;
    destNormalized = destSamples;
    for (auto& ch : refNormalized)
        for (auto& sample : ch)
            sample *= scale;
    for (auto& ch : destNormalized)
        for (auto& sample : ch)
            sample *= scale;
    return maxAmp;
}

void CompParamsCalculator::denormalize(std::vector<float>& result, float maxAmp)
{
    if (maxAmp <= 0.f || maxAmp == 1.f)
        return;
    const float thresholdOffsetDb = 20.f * std::log10(maxAmp);
    const int kneesNumber = ((int)result.size() - 1) / 3;
    for (int k = 0; k < kneesNumber; ++k)
        result[1 + 3 * k] += thresholdOffsetDb;
}