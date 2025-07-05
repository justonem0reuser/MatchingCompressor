#pragma once
#include <JuceHeader.h>
#include "../DSP/DynamicShaper.h"

// thresholdRange.interval and kneeWidthRange.interval must be 0!
const juce::NormalisableRange<float> 
	thresholdRange(-60.f, 0.f),
	ratioRange(-18.f, 20.f, 0.01f),
	gainRange(-50.f, 50.f, 0.01f),
	kneeWidthRange(0.0f, 20.f),
	attackRange(0.0f, 200.f, 0.01f),
	releaseRange(0.0f, 1000.f, 0.1f),
	
	// need to be float for 
	envelopeTypeRange(1, 2),
	channelAggregationTypeRange(1, 3),
	kneesNumberRange(1, DynamicShaper<float>::maxKneesNumber);
