#pragma once
#include <JuceHeader.h>

class PlotWithCoordinateSystemComponent : public juce::Component
{
public:
    PlotWithCoordinateSystemComponent(
        float leftMargin, float rightMargin,
        float topMargin, float bottomMargin,
        float leftPlotMargin, float rightPlotMargin, 
        float topPlotMargin, float bottomPlotMargin,
        juce::String xUnit, juce::String yUnit,
        juce::Colour backgroundColour,
        juce::Colour drawingColour);

    void initialize(
        float inputXMin, float inputXMax,
        float inputYMin, float inputYMax,
        float deltaX, float deltaY);

    void paint(juce::Graphics& g) override;
    void resized() override;

protected:
    float inputXMin, inputXMax, inputYMin, inputYMax,
        outputXMin, outputXMax, outputYMin, outputYMax,
        graphXMin, graphXMax, graphYMin, graphYMax,
        deltaX, deltaY, 
        leftMargin, rightMargin, topMargin, bottomMargin,
        leftPlotMargin, rightPlotMargin, topPlotMargin, bottomPlotMargin;
    juce::String xUnit, yUnit;
    juce::Colour backgroundColour, drawingColour;
    bool isReadyToDraw = false;

    float mapX(float x) const;
    float mapY(float y) const;
    void updateIsReadyToDraw();
    juce::Rectangle<int> getDrawingArea();
    void virtual paintBackground(juce::Graphics& g);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlotWithCoordinateSystemComponent)
};
