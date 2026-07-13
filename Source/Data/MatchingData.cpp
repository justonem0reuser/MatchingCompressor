#include "MatchingData.h"
#include "Messages.h"
#include "Ranges.h"

MatchingData::MatchingData():
	properties("properties"),
    initProperties(properties.getType())
{
    juce::StringArray kneesNumberChoices;
    for (int i = (int)kneesNumberRange.start; i <= (int)kneesNumberRange.end; i++)
        kneesNumberChoices.add(juce::String(i));

    parameterInfos.push_back(ParameterInfo(
        setKneesNumberId, "Knees number", 1, kneesNumberChoices));
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
