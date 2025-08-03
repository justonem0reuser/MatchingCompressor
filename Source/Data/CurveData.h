#pragma once
#include <JuceHeader.h>

/// <summary>
/// Data for drawing compressor/expander curves
/// </summary>
struct CurveData
{
public:
	juce::Path actualCompCurve, calculatedCompCurve;
	std::vector<juce::Point<float>> actualKneePoints;
};