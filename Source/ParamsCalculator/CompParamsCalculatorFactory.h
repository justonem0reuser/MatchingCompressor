#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include "CompParamsCalculator.h"

class CompParamsCalculatorFactory
{
public:
    static std::unique_ptr<CompParamsCalculator> create(
        const std::vector<std::vector<float>>& destSamples,
        const juce::ValueTree& properties);
};
