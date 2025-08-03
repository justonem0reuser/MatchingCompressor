#pragma once
#include <vector>
#include "ParameterInfo.h"

/// <summary>
/// All the data used by matching window
/// </summary>
struct MatchingData
{
public:
	MatchingData();
	
	/// <summary>
	/// Current reference and destination audio snaps
	/// </summary>
	std::vector<std::vector<float>> refSamples, destSamples;
	double refSampleRate = 0, destSampleRate = 0;

	/// <summary>
	/// New reference and destination audio snaps received from buses 
	/// before checking and saving.
	/// </summary>
	std::vector<std::vector<float>> newRefSamples, newDestSamples;
	double newRefSampleRate = 0, newDestSampleRate = 0;

	/// <summary>
	/// Matching window parameters information
	/// used for automated creation of matching window component
	/// </summary>
	std::vector<ParameterInfo> parameterInfos;

	/// <summary>
	/// Current matching window parameters
	/// </summary>
	juce::ValueTree properties;

	/// <summary>
	/// Initial matching window parameters 
	/// in case of Cancel button pressing
	/// </summary>
	juce::ValueTree initProperties;

	/// <summary>
	/// Compression parameters calculated from snaps
	/// </summary>
	std::vector<float> calculatedCompParams;
};
