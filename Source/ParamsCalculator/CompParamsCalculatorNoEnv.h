#pragma once

#include <JuceHeader.h>
#include "CompParamsCalculator.h"

class CompParamsCalculatorNoEnv : public CompParamsCalculator
{
public:
    std::vector<float> calculateCompressorParameters(
        std::vector<std::vector<float>>& refSamples, 
        double refSampleRate,
        std::vector<std::vector<float>>& destSamples, 
        double destSampleRate,
        juce::ValueTree& properties) override;

protected:
    const double epsx = 0.000001;
    const alglib::ae_int_t maxits = 0;
};