#pragma once

class FuncAndGradCalculator
{
public:
    static double calculateWithoutGain(
        double levelDb,
        const double* c,
        int kneesNumber,
        bool convertResultToLinear = false,
        double* grad = nullptr,
        const double* widths = nullptr,
        const double* dWidthDThreshold = nullptr);
};
