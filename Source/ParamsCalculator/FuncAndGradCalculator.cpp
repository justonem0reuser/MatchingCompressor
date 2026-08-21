#include "FuncAndGradCalculator.h"
#include <JuceHeader.h>

double FuncAndGradCalculator::calculateWithoutGain(
    double levelDb,
    const double* c,
    int kneesNumber,
    bool convertResultToLinear,
    double* grad,
    const double* widths,
    const double* dWidthDThreshold)
{
    //c : Gain (doesn't used), [Threshold, 1/Ratio (, Knee weight)] * n

    const int stride = widths != nullptr ? 2 : 3;

    double res = 0;

    int index = -1;
    for (int i = 0; i < kneesNumber; i++)
    {
        auto iS = stride * i;
        auto curThreshold = c[iS + 1];
        auto curKnee = widths != nullptr ? widths[i] : c[iS + 3];

        bool isBandStarted = levelDb > curThreshold - 0.5 * curKnee;
        index = isBandStarted ? i : index;
        if (i > 0 && isBandStarted)
            res += (curThreshold - c[iS + 1 - stride]) * (c[iS + 2 - stride] - 1);
        if (grad != nullptr)
        {
            grad[iS + 1] = grad[iS + 2] = 0.0;
            if (widths == nullptr)
                grad[iS + 3] = 0.0;
            if (i > 0 && isBandStarted)
            {
                auto prevRatioInvM1 = c[iS + 2 - stride] - 1;
                grad[iS + 1] += prevRatioInvM1;
                grad[iS + 1 - stride] -= prevRatioInvM1;
                grad[iS + 2 - stride] += curThreshold - c[iS + 1 - stride];
            }
        }
    }

    if (index == -1)
        res += levelDb;
    else
    {
        auto iS = stride * index;
        auto prevRatioInv = index == 0 ? 1.0 : c[iS + 2 - stride];
        auto curThreshold = c[iS + 1];
        auto curRatioInv = c[iS + 2];
        auto curKneeWidth = widths != nullptr ? widths[index] : c[iS + 3];
        auto curLeftBound = curThreshold - 0.5 * curKneeWidth;

        if (levelDb >= curThreshold + 0.5 * curKneeWidth)
        {
            res += curThreshold + (levelDb - curThreshold) * curRatioInv;
            if (grad != nullptr)
            {
                grad[iS + 1] += 1 - curRatioInv;
                grad[iS + 2] += levelDb - curThreshold;
            }
        }
        else
        {
            auto kneeOffset = levelDb - curLeftBound;
            auto kneePos = kneeOffset / curKneeWidth;
            auto ratioInvDelta = curRatioInv - prevRatioInv;

            res += curThreshold - 0.5 * prevRatioInv * curKneeWidth +
                prevRatioInv * kneeOffset +
                0.5 * ratioInvDelta * kneeOffset * kneePos;

            if (grad != nullptr)
            {
                grad[iS + 1] += 1.0 - prevRatioInv - ratioInvDelta * kneePos;
                grad[iS + 2] += 0.5 * kneeOffset * kneePos;

                auto dResDWidth = 0.5 * ratioInvDelta * kneePos * (1.0 - kneePos);
                if (widths == nullptr)
                    grad[iS + 3] += dResDWidth;
                else
                {
                    jassert(dWidthDThreshold != nullptr);
                    auto dWidth = dWidthDThreshold != nullptr ? dWidthDThreshold[index] : 0.0;
                    if (dWidth != 0.0)
                    {
                        int neighbour = dWidth > 0.0 ? index - 1 : index + 1;
                        jassert(neighbour >= 0 && neighbour < kneesNumber);
                        grad[iS + 1] += dResDWidth * dWidth;
                        grad[1 + stride * neighbour] -= dResDWidth * dWidth;
                    }
                }

                if (index > 0)
                    grad[iS + 2 - stride] += kneeOffset - 0.5 * curKneeWidth -
                        0.5 * kneeOffset * kneePos;
            }
        }
    }

    if (convertResultToLinear)
    {
        res = juce::Decibels::decibelsToGain(res);
        double coeff = 0.05 * std::log(10.0) * res;
        if (grad != nullptr)
            for (int i = 1; i < stride * kneesNumber + 1; i++)
                grad[i] *= coeff;
    }

    return res;
}
