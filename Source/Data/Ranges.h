#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>
#include "../DSP/DynamicShaper.h"

// thresholdRange.interval and kneeWidthRange.interval must be 0!
const juce::NormalisableRange<float>
	thresholdRange(-60.f, 0.f),
	gainRange(-50.f, 50.f, 0.01f),
	kneeWidthRange(0.0f, 20.f),
	attackRange(0.0f, 200.f, 0.01f),
	releaseRange(0.0f, 1000.f, 0.1f),

	// need to be float for
	envelopeTypeRange(1, 2),
	channelAggregationTypeRange(1, 3),
	kneesNumberRange(1, DynamicShaper<float>::maxKneesNumber);

namespace RatioSliderLaw
{
	inline constexpr float
		// ratio and ratioInverse share these limits, the range being symmetric
		maxRatio = 20.f,
		minRatio = 1.f / maxRatio,
		
		halfSpan = maxRatio - 1.f,
		step = 0.01f;

	inline float from0to1(float, float, float position)
	{
		return position <= 0.5f ?
			1.f + (0.5f - position) * 2.f * halfSpan :
			1.f / (1.f + (position - 0.5f) * 2.f * halfSpan);
	}

	inline float to0to1(float rangeStart, float rangeEnd, float ratioInverse)
	{
		ratioInverse = std::clamp(ratioInverse, rangeStart, rangeEnd);
		return ratioInverse >= 1.f ?
			0.5f - (ratioInverse - 1.f) / (2.f * halfSpan) :
			0.5f + (1.f / ratioInverse - 1.f) / (2.f * halfSpan);
	}

	inline float snap(float rangeStart, float rangeEnd, float ratioInverse)
	{
		ratioInverse = std::clamp(ratioInverse, rangeStart, rangeEnd);
		float snapped = ratioInverse >= 1.f ?
			std::round(ratioInverse / step) * step :
			1.f / (std::round(1.f / ratioInverse / step) * step);
		return std::clamp(snapped, rangeStart, rangeEnd);
	}
}

const juce::NormalisableRange<float> ratioInverseRange(
	RatioSliderLaw::minRatio, RatioSliderLaw::maxRatio,
	RatioSliderLaw::from0to1, RatioSliderLaw::to0to1, RatioSliderLaw::snap);
