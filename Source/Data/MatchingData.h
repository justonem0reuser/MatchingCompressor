#pragma once
#include <vector>
#include "ParameterInfo.h"

struct MatchingData
{
public:
	MatchingData();
	std::vector<std::vector<float>> refSamples, destSamples;
	double refSampleRate = 0, destSampleRate = 0;

	std::vector<std::vector<float>> newRefSamples, newDestSamples;
	double newRefSampleRate = 0, newDestSampleRate = 0;

	std::vector<ParameterInfo> parameterInfos;

	juce::ValueTree properties, initProperties;

	std::vector<float> calculatedCompParams;
};
