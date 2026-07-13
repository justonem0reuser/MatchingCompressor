#pragma once

#include "alglibinternal.h"

class FuncAndGradCalculator
{
public:
    static double calculateWithoutGain(
        double levelDb,
        const alglib::real_1d_array& c,
        alglib::real_1d_array* grad = nullptr);
};
