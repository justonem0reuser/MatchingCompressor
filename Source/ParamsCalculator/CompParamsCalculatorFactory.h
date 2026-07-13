#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include "CompParamsCalculator.h"
#include "CompParamsCalculatorNoEnv.h"
#include "CompParamsCalculatorEnv.h"
#include "../Data/Messages.h"

class CompParamsCalculatorFactory
{
public:
    static std::unique_ptr<CompParamsCalculator> create(
        const std::vector<std::vector<float>>& destSamples,
        const juce::ValueTree& properties)
    {
        float attackMs = properties.getProperty(setAttackId);
        float releaseMs = properties.getProperty(setReleaseId);
        int channelAggregationTypeInt = properties.getProperty(setChannelAggregationTypeId);

        if (attackMs == 0 && releaseMs == 0 &&
            (destSamples.size() == 1 || channelAggregationTypeInt == 1))
            return std::make_unique<CompParamsCalculatorNoEnv>();
        else
            return std::make_unique<CompParamsCalculatorEnv>();
    }
};
