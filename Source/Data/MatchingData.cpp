#include "MatchingData.h"
#include "Messages.h"
#include "Ranges.h"
#include "../DSP/DynamicShaper.h"

MatchingData::MatchingData():
	properties("properties"),
    initProperties(properties.getType())
{
    // Sharp accuracy decreasing below -60 dB when optimizing in dBs;
    // accuracy doesn't decreased when optimizing in gain units instead of dBs.
    // For hard-knee optimization gainRegionsNumber=100, quantileRegionsNumber=1000 are enough
    // and it makes the process significantly faster;
    // For soft-knee optimization values around gainRegionsNumber=500, quantileRegionsNumber=5000 are needed.

    parameterInfos.push_back(ParameterInfo(
        setGainRegionsNumberId, "Gain regions number", 4000, 1000, 5000, 1, false, true));
    parameterInfos.push_back(ParameterInfo(
        setQuantileRegionsNumberId, "Quantile regions number", 400, 100, 1000, 1, false, true));
    parameterInfos.push_back(ParameterInfo(
        setKneesNumberId, "Knees number", 1, 1, DynamicShaper<float>::maxKneesNumber, 1, false, true));
    parameterInfos.push_back(ParameterInfo(
        setKneeTypeId, "Knee type", 1, kneeTypes));
    parameterInfos.push_back(ParameterInfo(
        setBalFilterTypeId, "Envelope type", 1, balFilterTypes));
    parameterInfos.push_back(ParameterInfo(
        setChannelAggregationTypeId, "Stereo processing", 1, channelAggregationTypes));
    parameterInfos.push_back(ParameterInfo(
        setAttackId, "Attack (ms)", 10.f, attackRange.start, attackRange.end, attackRange.interval, false, true));
    parameterInfos.push_back(ParameterInfo(
        setReleaseId, "Release (ms)", 100.f, releaseRange.start, releaseRange.end, releaseRange.interval, false, true));

    for (int i = 0; i < parameterInfos.size(); i++)
        properties.setProperty(
            parameterInfos[i].name, parameterInfos[i].defaultValue, nullptr);
    initProperties.copyPropertiesFrom(properties, nullptr);
}
