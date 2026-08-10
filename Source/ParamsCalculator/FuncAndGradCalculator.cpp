#include "FuncAndGradCalculator.h"
#include <JuceHeader.h>

double FuncAndGradCalculator::calculateWithoutGain(
    double levelDb,
    const double* c,
    int kneesNumber,
    bool convertResultToLinear,
    double* grad)
{
    //c : Gain (doesn't used), [Threshold, 1/Ratio, Knee weight] * n

    double res = 0;

    int index = -1;
    for (int i = 0; i < kneesNumber; i++)
    {
        auto i3 = 3 * i;
        auto curThreshold = c[i3 + 1];
        auto curKnee = c[i3 + 3];

        bool isBandStarted = levelDb > curThreshold - 0.5 * curKnee;
        index = isBandStarted ? i : index;
        if (i > 0 && isBandStarted)
            res += (curThreshold - c[i3 - 2]) * (c[i3 - 1] - 1);
        if (grad != nullptr)
        {
            grad[i3 + 1] = grad[i3 + 2] = grad[i3 + 3] = 0.0;
            if (i > 0 && isBandStarted)
            {
                auto prevRatioInvM1 = c[i3 - 1] - 1;
                grad[i3 + 1] += prevRatioInvM1;
                grad[i3 - 2] -= prevRatioInvM1;
                grad[i3 - 1] += curThreshold - c[i3 - 2];
            }
        }
    }

    if (index == -1)
        res += levelDb;
    else
    {
        auto i3 = 3 * index;
        auto prevRatioInv = index == 0 ? 1.0 : c[i3 - 1];
        auto curThreshold = c[i3 + 1];
        auto curRatioInv = c[i3 + 2];
        auto curKneeWidth = c[i3 + 3];
        auto curLeftBound = curThreshold - 0.5 * curKneeWidth;

        if (levelDb >= curThreshold + 0.5 * curKneeWidth)
        {
            res += curThreshold + (levelDb - curThreshold) * curRatioInv;
            if (grad != nullptr)
            {
                grad[i3 + 1] += 1 - curRatioInv;
                grad[i3 + 2] += levelDb - curThreshold;
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
                grad[i3 + 1] += 1.0 - prevRatioInv - ratioInvDelta * kneePos;
                grad[i3 + 2] += 0.5 * kneeOffset * kneePos;
                grad[i3 + 3] += 0.5 * ratioInvDelta * kneePos * (1.0 - kneePos);

                if (index > 0)
                    grad[i3 - 1] += kneeOffset - 0.5 * curKneeWidth -
                    0.5 * kneeOffset * kneePos;
            }
        }
    }

    if (convertResultToLinear)
    {
        res = juce::Decibels::decibelsToGain(res);
        double coeff = 0.05 * std::log(10.0) * res;
        if (grad != nullptr)
            for (int i = 1; i < 3 * kneesNumber + 1; i++)
                grad[i] *= coeff;
    }

    return res;
}
