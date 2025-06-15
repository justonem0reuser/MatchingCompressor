#include "PlotWithCoordinateSystemComponent.h"

PlotWithCoordinateSystemComponent::PlotWithCoordinateSystemComponent(
	float leftMargin, float rightMargin,
	float topMargin, float bottomMargin,
	float leftPlotMargin, float rightPlotMargin,
	float topPlotMargin, float bottomPlotMargin,
	juce::String xUnit, juce::String yUnit,
	juce::Colour backgroundColour,
	juce::Colour drawingColour):
	leftMargin(leftMargin), rightMargin(rightMargin),
	topMargin(topMargin), bottomMargin(bottomMargin),
	leftPlotMargin(leftPlotMargin), rightPlotMargin(rightPlotMargin),
	topPlotMargin(topPlotMargin), bottomPlotMargin(bottomPlotMargin),
	xUnit(xUnit), yUnit(yUnit),
	backgroundColour(backgroundColour),	drawingColour(drawingColour)
{ 
	jassert(leftMargin >= 0.f && rightMargin >= 0.f &&
		topMargin >= 0.f && bottomMargin >= 0.f &&
		leftPlotMargin >= 0.f && rightPlotMargin >= 0.f &&
		topPlotMargin >= 0.f && bottomPlotMargin >= 0.f);
}

void PlotWithCoordinateSystemComponent::initialize(
	float inputXMin, float inputXMax, 
	float inputYMin, float inputYMax, 
	float deltaX, float deltaY)
{
	jassert(inputXMin < inputXMax && inputYMin < inputYMax &&
		deltaX > 0.f && deltaY > 0.f);
	this->inputXMin = inputXMin;
	this->inputXMax = inputXMax;
	this->inputYMin = inputYMin;
	this->inputYMax = inputYMax;
	this->deltaX = deltaX;
	this->deltaY = deltaY;
	updateIsReadyToDraw();
}

void PlotWithCoordinateSystemComponent::paint(juce::Graphics& g)
{
	paintBackground(g);
	if (isReadyToDraw)
	{
		juce::Line<float> xAxis(graphXMin, graphYMin, graphXMax, graphYMin),
			yAxis(graphXMin, graphYMin, graphXMin, graphYMax);
		g.setColour(drawingColour);
		g.drawArrow(xAxis, 1.f, 5.f, 15.f);
		g.drawArrow(yAxis, 1.f, 5.f, 15.f);
		g.drawSingleLineText(xUnit, outputXMax - rightMargin - 15, outputYMin - bottomMargin - 2);
		g.drawSingleLineText(yUnit, outputXMin + leftMargin + 2, outputYMax + topMargin + 10);

		for (float i = inputXMin; i <= inputXMax; i += deltaX)
		{
			g.drawVerticalLine(mapX(i), graphYMax, graphYMin);
			g.drawSingleLineText(std::to_string((int)i), mapX(i) - 10, outputYMin - bottomMargin - 2);
		}

		for (float i = inputYMin; i <= inputYMax; i += deltaY)
		{
			g.drawHorizontalLine(mapY(i), graphXMin, graphXMax);
			g.drawSingleLineText(std::to_string((int)i), outputXMin + leftMargin + 2, mapY(i) + 5);
		}
	}
}

void PlotWithCoordinateSystemComponent::resized()
{
	auto bounds = getLocalBounds();
	outputXMin = bounds.getX();
	outputXMax = bounds.getRight();
	outputYMin = bounds.getBottom();
	outputYMax = bounds.getY();
	graphXMin = outputXMin + leftMargin + leftPlotMargin;
	graphXMax = outputXMax - rightMargin - rightPlotMargin;
	graphYMin = outputYMin - bottomMargin - bottomPlotMargin;
	graphYMax = outputYMax + topMargin + topPlotMargin;
	updateIsReadyToDraw();
}

float PlotWithCoordinateSystemComponent::mapX(float x) const
{
	return juce::jmap(x, inputXMin, inputXMax, graphXMin, graphXMax);
}

float PlotWithCoordinateSystemComponent::mapY(float y) const
{
	return juce::jmap(y, inputYMin, inputYMax, graphYMin, graphYMax);
}

void PlotWithCoordinateSystemComponent::updateIsReadyToDraw()
{
	isReadyToDraw =
		inputXMin < inputXMax && inputYMin < inputYMax &&
		graphXMin < graphXMax && graphYMin > graphYMax && // >!
		deltaX > 0.f && deltaY > 0.f;
}

juce::Rectangle<int> PlotWithCoordinateSystemComponent::getDrawingArea()
{
	auto bounds = getLocalBounds();
	bounds.removeFromLeft(leftMargin + leftPlotMargin);
	bounds.removeFromRight(rightMargin + rightPlotMargin);
	bounds.removeFromTop(topMargin + topPlotMargin);
	bounds.removeFromBottom(bottomMargin + bottomPlotMargin);
	return bounds;
}

void PlotWithCoordinateSystemComponent::paintBackground(juce::Graphics& g)
{
	g.fillAll(backgroundColour);
}
