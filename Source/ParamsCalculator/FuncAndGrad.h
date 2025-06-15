#pragma once
#include "alglibinternal.h"
#include "CompParamsCalculatorEnv.h"

static class FuncAndGrad
{
public:
    static void comp_func(
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

    static void comp_func_db(
        const alglib::real_1d_array& c, 
        const alglib::real_1d_array& x, 
        double& func, 
        void* ptr)
    {
        func = comp_func_grad_db_wo_fine(c, x) + calculateFine(c);
    }

    static void comp_grad_db(
        const alglib::real_1d_array& c,
        const alglib::real_1d_array& x,
        double& func,
        alglib::real_1d_array& grad,
        void* ptr)
    {
        func = comp_func_grad_db_wo_fine(c, x, &grad) + calculateFine(c, &grad);
    }

    static double calculateFine(
        const alglib::real_1d_array& c,
        alglib::real_1d_array* gradPtr = nullptr)
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

            auto curKnee =
                c[curKneeInd] >= DynamicShaper<float>::minKneeWidth ?
                c[curKneeInd] : 0.0;
            auto prevKnee =
                c[prevKneeInd] >= DynamicShaper<float>::minKneeWidth ?
                c[prevKneeInd] : 0.0;

            auto curLeftBound = c[curTInd] - 0.5 * curKnee;
            auto prevRightBound = c[prevTInd] + 0.5 * prevKnee;

            auto fineDelta = curLeftBound - prevRightBound - fineThreshold;
            if (fineDelta < 0.0)
            {
                fine += fineCoeff * fineDelta * fineDelta;
                if (gradPtr != nullptr)
                {
                    auto grad = *gradPtr;
                    double valueToAdd = 2.0 * fineCoeff * fineDelta;
                    grad[prevTInd] += valueToAdd;
                    grad[prevKneeInd] += valueToAdd;
                    grad[curTInd] -= valueToAdd; // -
                    grad[curKneeInd] += valueToAdd;
                }
            }
        }
        return fine;
    }

    friend class Tests;

private:
    constexpr static double fineThreshold = 0.1;
    constexpr static double fineCoeff = 100.0;
    constexpr static double minusInfinityDb = DynamicShaper<double>::minusInfinityDb;

    static double comp_func_grad_db_wo_fine(
        const alglib::real_1d_array& c, 
        const alglib::real_1d_array& x, 
        alglib::real_1d_array* gradPtr = nullptr)
    {
        //c : Gain, [Threshold, 1/Ratio, Knee weight] * n

        double func = c[0];
        if (gradPtr != nullptr)
            (*gradPtr)[0] = 1.0;

        const int cLength = c.length();
        const int size = (cLength - 1) / 3;
        const double xDb = juce::Decibels::gainToDecibels(x[0], minusInfinityDb);

        int index = -1;
        for (int i = 0; i < size; i++)
        {
            auto i3 = 3 * i;
            auto curThreshold = c[i3 + 1];
            auto curKnee = 
                c[i3 + 3] >= DynamicShaper<float>::minKneeWidth ?
                c[i3 + 3] : 0.0;

            int mask = (int)(xDb > curThreshold - 0.5 * curKnee);
            index += mask;
            if (i > 0)
                func += mask * (curThreshold - c[i3 - 2]) * (c[i3 - 1] - 1);
            if (gradPtr != nullptr)
            {
                (*gradPtr)[i3 + 1] = (*gradPtr)[i3 + 2] = (*gradPtr)[i3 + 3] = 0.0;
                if (i > 0 && mask != 0)
                {
                    auto prevRatioInvM1 = c[i3 - 1] - 1;
                    (*gradPtr)[i3 + 1] += prevRatioInvM1;
                    (*gradPtr)[i3 - 2] -= prevRatioInvM1;
                    (*gradPtr)[i3 - 1] += curThreshold - c[i3 - 2];
                }
            }
        }

        if (index == -1)
        {
            func += xDb;
            dbToGain(cLength, func, gradPtr);
            return func;
        }

        auto i3 = 3 * index;
        auto prevRatioInv = index == 0 ? 1.0 : c[i3 - 1];
        auto curThreshold = c[i3 + 1];
        auto curRatioInv = c[i3 + 2];
        auto curKneeWidth = 
            c[i3 + 3] >= DynamicShaper<float>::minKneeWidth ?
            c[i3 + 3] : 0.0;
        auto curLeftBound = curThreshold - 0.5 * curKneeWidth;

        if (xDb >= curThreshold + 0.5 * curKneeWidth)
        {
            func += curThreshold + (xDb - curThreshold) * curRatioInv;
            if (gradPtr != nullptr)
            {
                (*gradPtr)[i3 + 1] += 1 - curRatioInv;
                (*gradPtr)[i3 + 2] += xDb - curThreshold;
            }
        }
        else
        {
            auto aQuadCoeff = 0.5 * (curRatioInv - prevRatioInv) / curKneeWidth;
            auto bQuadCoeff = prevRatioInv - 2.0 * aQuadCoeff * curLeftBound;
            auto cQuadCoeff = curThreshold -
                0.5 * prevRatioInv * curKneeWidth -
                curLeftBound * (curLeftBound * aQuadCoeff + bQuadCoeff);

            func += xDb * (xDb * aQuadCoeff + bQuadCoeff) + cQuadCoeff;

            if (gradPtr != nullptr)
            {
                auto dAdT = 0.0;
                auto dAdRInv = 0.5 / curKneeWidth;
                auto dAdKW = (prevRatioInv - curRatioInv) * dAdRInv / curKneeWidth;

                auto dBdT = -2.0 * aQuadCoeff;
                auto dBdRInv = -2.0 * curLeftBound * dAdRInv;
                auto dBdKW = aQuadCoeff -2.0 * curLeftBound * dAdKW;

                auto dCdT = 1.0 - bQuadCoeff -
                    curLeftBound * (2.0 * aQuadCoeff + curLeftBound * dAdT + dBdT);
                auto dCdRInv = -curLeftBound * (curLeftBound * dAdRInv + dBdRInv);
                auto dCdKW = 0.5 * (bQuadCoeff - prevRatioInv) +
                    curLeftBound * (aQuadCoeff - curLeftBound * dAdKW - dBdKW);

                (*gradPtr)[i3 + 1] += xDb * (xDb * dAdT + dBdT) + dCdT;
                (*gradPtr)[i3 + 2] += xDb * (xDb * dAdRInv + dBdRInv) + dCdRInv;
                (*gradPtr)[i3 + 3] += xDb * (xDb * dAdKW + dBdKW) + dCdKW;

                if (index > 0)
                {
                    auto dAdPrevRInv = -dAdRInv;
                    auto dBdPrevRInv = 1 - 2.0 * curLeftBound * dAdPrevRInv;
                    auto dCdPrevRInv = -0.5 * curKneeWidth -
                        curLeftBound * (curLeftBound * dAdPrevRInv + dBdPrevRInv);
                    (*gradPtr)[i3 - 1] += xDb * (xDb * dAdPrevRInv + dBdPrevRInv) + dCdPrevRInv;
                }
            }
        }

        dbToGain(cLength, func, gradPtr);
        return func;
    }

    static void dbToGain(
        int cLength,
        double& func, 
        alglib::real_1d_array* gradPtr = nullptr)
    {
        func = juce::Decibels::decibelsToGain(func, minusInfinityDb);
        if (gradPtr != nullptr)
        {
            const double coeff = std::log(std::pow(10.0, 0.05));
            for (int i = 0; i < cLength; i++)
                (*gradPtr)[i] *= func * coeff;
        }
    }
};