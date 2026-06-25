#include "CompParamsCalculatorFactory.h"
#include "CompParamsCalculatorNoEnv.h"
#include "CompParamsCalculatorEnv1D.h"
#include "CompParamsCalculatorEnv2D.h"
#include "../Data/Messages.h"

std::unique_ptr<CompParamsCalculator> CompParamsCalculatorFactory::create(
    const std::vector<std::vector<float>>& destSamples,
    const juce::ValueTree& properties)
{
    float attackMs = properties.getProperty(setAttackId);
    float releaseMs = properties.getProperty(setReleaseId);
    int channelAggregationTypeInt = properties.getProperty(setChannelAggregationTypeId);
    int gainRegionsNumber = properties.getProperty(setGainRegionsNumberId);

    if (attackMs == 0 && releaseMs == 0 &&
        (destSamples.size() == 1 || channelAggregationTypeInt == 1))
        return std::make_unique<CompParamsCalculatorNoEnv>();
    else if (destSamples[0].size() * destSamples.size() < gainRegionsNumber)
        return std::make_unique<CompParamsCalculatorEnv1D>();
    else
        return std::make_unique<CompParamsCalculatorEnv2D>();
}
