#include "FuncAndGradCalculator.h"
#include "../DSP/DynamicShaper.h"

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
        auto curKnee =
            c[i3 + 3] >= DynamicShaper<float>::minKneeWidth ?
            c[i3 + 3] : 0.0;

        int mask = (int)(levelDb > curThreshold - 0.5 * curKnee);
        index += mask;
        if (i > 0)
            res += mask * (curThreshold - c[i3 - 2]) * (c[i3 - 1] - 1);
        if (grad != nullptr)
        {
            grad[i3 + 1] = grad[i3 + 2] = grad[i3 + 3] = 0.0;
            if (i > 0 && mask != 0)
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
        auto curKneeWidth =
            c[i3 + 3] >= DynamicShaper<float>::minKneeWidth ?
            c[i3 + 3] : 0.0;
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
            auto aQuadCoeff = 0.5 * (curRatioInv - prevRatioInv) / curKneeWidth;
            auto bQuadCoeff = prevRatioInv - 2.0 * aQuadCoeff * curLeftBound;
            auto cQuadCoeff = curThreshold -
                0.5 * prevRatioInv * curKneeWidth -
                curLeftBound * (curLeftBound * aQuadCoeff + bQuadCoeff);

            res += levelDb * (levelDb * aQuadCoeff + bQuadCoeff) + cQuadCoeff;

            if (grad != nullptr)
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

                grad[i3 + 1] += levelDb * (levelDb * dAdT + dBdT) + dCdT;
                grad[i3 + 2] += levelDb * (levelDb * dAdRInv + dBdRInv) + dCdRInv;
                grad[i3 + 3] += levelDb * (levelDb * dAdKW + dBdKW) + dCdKW;

                if (index > 0)
                {
                    auto dAdPrevRInv = -dAdRInv;
                    auto dBdPrevRInv = 1 - 2.0 * curLeftBound * dAdPrevRInv;
                    auto dCdPrevRInv = -0.5 * curKneeWidth -
                        curLeftBound * (curLeftBound * dAdPrevRInv + dBdPrevRInv);
                    grad[i3 - 1] += levelDb * (levelDb * dAdPrevRInv + dBdPrevRInv) + dCdPrevRInv;
                }
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
