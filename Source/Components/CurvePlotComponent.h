#pragma once
#include <JuceHeader.h>
#include "PlotWithCoordinateSystemComponent.h"

class CurvePlotComponent : public PlotWithCoordinateSystemComponent
{
public:
    constexpr static float margin = 5.f;
    constexpr static float plotMargin = 30.f;

    CurvePlotComponent(
        const juce::Colour gridColour,
        const juce::Colour calculatedCurveColor,
        const juce::Colour actualCurveColor,
        const juce::Colour thresholdVerticalLineColor);

    void setData(std::vector<float>& compParams);
    void updateActualParameters(
        juce::AudioProcessorValueTreeState& apvts, 
        int kneesNumber);
    void paint(juce::Graphics& g) override;

private:
    const float plotThreshold = -60.f;
    const float dashedLineLengths[2] { 4, 4 };

    juce::Colour calculatedCurveColor, actualCurveColor, thresholdVerticalLineColor;

    std::vector<float> actualCompParams, calculatedCompParams;
    juce::Path actualCompCurve, calculatedCompCurve;
    
    juce::Path calculateCurve(
        std::vector<float>& compParams,
        std::vector<juce::Point<float>>& kneePoints);
    void paintBackground(juce::Graphics& g) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurvePlotComponent)
};