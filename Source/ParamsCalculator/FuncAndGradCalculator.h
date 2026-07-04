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
        const alglib::real_1d_array& c,
        bool convertResultToLinear = false,
        alglib::real_1d_array* grad = nullptr);
};
