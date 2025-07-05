#pragma once
#include <JuceHeader.h>
#include "../Data/CurveBounds.h"

class CurveCalculator
{
public:
    CurveCalculator(CurveBounds& curveBounds);
    juce::Path calculateCurve(
        std::vector<float>& compParams,
        std::vector<juce::Point<float>>& kneePoints);
private:
    CurveBounds& curveBounds;
    float mapX(float x) const;
    float mapY(float y) const;
};