#pragma once

#include "alglibinternal.h"

/// <summary>
/// Evaluates the static gain-computer curve (excluding make-up gain) 
/// and (optionally) its analytic gradient.
/// </summary>
class FuncAndGradCalculator
{
public:
    static double calculateWithoutGain(
        double levelDb,
        const double* c,
        int kneesNumber,
        bool convertResultToLinear = false,
        double* grad = nullptr);
};
