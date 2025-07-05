#pragma once
#include <JuceHeader.h>

struct CurveData
{
public:
	juce::Path actualCompCurve, calculatedCompCurve;
	std::vector<juce::Point<float>> actualKneePoints;
};