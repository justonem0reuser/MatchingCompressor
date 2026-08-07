#include "CurvePlotComponent.h"
#include "../Data/Messages.h"
#include "../ParamsCalculator/FuncAndGradCalculator.h"

CurvePlotComponent::CurvePlotComponent() :
    PlotWithCoordinateSystemComponent(
        margin, margin, margin, margin,
        plotMargin, plotMargin, plotMargin, plotMargin,
        "dB", "dB")
{
}

void CurvePlotComponent::setData(std::vector<float>& compParams)
{ 
    this->calculatedCompParams = compParams;
    actualCompParams.assign(compParams.begin(), compParams.end());
    initialize(plotThreshold, 0.f, plotThreshold, 0.f, 10.f, 10.f);
}

void CurvePlotComponent::updateActualParameters(
    juce::AudioProcessorValueTreeState& apvts, 
    int kneesNumber)
{ 
    if (kneesNumber > 0)
    {
        actualCompParams.resize(3 * kneesNumber + 1);
        auto gPar = apvts.getParameter(gainId);
        actualCompParams[0] = gPar->convertFrom0to1(gPar->getValue());
        
        for (int i = 0; i < kneesNumber; i++)
        {
            auto tPar = apvts.getParameter(thresholdId + std::to_string(i));
            actualCompParams[i * 3 + 1] = tPar->convertFrom0to1(tPar->getValue());
            
            auto rPar = apvts.getParameter(ratioId + std::to_string(i));
            auto r = rPar->convertFrom0to1(rPar->getValue());
            if (r < 1.f)
                r = 1.f / (2.f - r);
            actualCompParams[i * 3 + 2] = r;

            auto kPar = apvts.getParameter(kneeWidthId + std::to_string(i));
            actualCompParams[i * 3 + 3] = kPar->convertFrom0to1(kPar->getValue());
        }
        repaint();
    }
}

void CurvePlotComponent::paint(juce::Graphics& g)
{
    if (isReadyToDraw)
    {
        PlotWithCoordinateSystemComponent::paint(g);

        auto responseArea = getDrawingArea();
        juce::Graphics::ScopedSaveState sss(g);
        g.reduceClipRegion(responseArea);

        std::vector<juce::Point<float>> kneePoints;

        calculatedCompCurve = calculateCurve(calculatedCompParams, kneePoints);
        g.setColour(findColour(MCLookAndFeel::calculatedCompCurveColourId));
        g.strokePath(calculatedCompCurve, juce::PathStrokeType(1.f));

        actualCompCurve = calculateCurve(actualCompParams, kneePoints);
        g.setColour(findColour(MCLookAndFeel::actualCompCurveColourId));
        g.strokePath(actualCompCurve, juce::PathStrokeType(3.f));

        g.setColour(findColour(MCLookAndFeel::thresholdVerticalLineColourId));
        for (int i = 1; i < actualCompParams.size(); i += 3)
        {
            auto x = mapX(actualCompParams[i]);
            g.drawDashedLine({ x, graphYMax, x, graphYMin }, dashedLineLengths, 2);
        }

        for (auto p: kneePoints)
            g.drawDashedLine({ p.x, p.y - 10, p.x, p.y + 10 }, dashedLineLengths, 2);
    }
}

juce::Path CurvePlotComponent::calculateCurve(
    std::vector<float>& compParams,
    std::vector<juce::Point<float>>& kneePoints)
{
    kneePoints.clear();
    auto compParamsSize = compParams.size();
    if (compParamsSize < 4 ||
        (compParamsSize - 1) % 3 != 0)
        return {};
    auto size = (compParamsSize - 1) / 3;

    std::vector<double> coreParams(compParamsSize);
    coreParams[0] = compParams[0];
    for (int i = 0; i < size; i++)
    {
        if (compParams[3 * i + 1] > 0.f ||
            compParams[3 * i + 2] <= 0.f ||
            compParams[3 * i + 3] < 0.f)
            return {};
        coreParams[3 * i + 1] = compParams[3 * i + 1];
        coreParams[3 * i + 2] = 1.0 / compParams[3 * i + 2];
        coreParams[3 * i + 3] = compParams[3 * i + 3];
    }

    float compGain = compParams[0];
    auto levelToOutput = [&](float levelDb)
    {
        return (float)FuncAndGradCalculator::calculateWithoutGain(
            levelDb, coreParams.data(), (int)size) + compGain;
    };

    juce::Path curve;
    for (int i = 0; i < size; i++)
    {
        float compThreshold = compParams[3 * i + 1];
        float compKneeWidth = compParams[3 * i + 3];
        float kneeStart = compThreshold - 0.5f * compKneeWidth;
        float kneeEnd = kneeStart + compKneeWidth;

        if (i == 0)
        {
            float startLevel = std::min(inputXMin, kneeStart);
            curve.startNewSubPath(mapX(startLevel), mapY(levelToOutput(startLevel)));
            curve.lineTo(mapX(kneeStart), mapY(levelToOutput(kneeStart)));
        }

        if (compKneeWidth > 0)
        {
            kneePoints.push_back(curve.getCurrentPosition());

            float dx = -inputXMin / (graphXMax - graphXMin);
            if (compParams[3 * i + 2] < 1.f)
                dx *= compParams[3 * i + 2];
            for (float x = kneeStart + dx; x < kneeEnd && x < 0.f; x += dx)
                curve.lineTo(mapX(x), mapY(levelToOutput(x)));

            if (kneeEnd <= 0.f)
            {
                curve.lineTo(mapX(kneeEnd), mapY(levelToOutput(kneeEnd)));
                kneePoints.push_back(curve.getCurrentPosition());
            }
        }

        float rightBoundX = i == size - 1 ? inputXMax :
            compParams[3 * i + 4] - 0.5f * compParams[3 * i + 6];
        curve.lineTo(mapX(rightBoundX), mapY(levelToOutput(rightBoundX)));
    }
    return curve;
}

void CurvePlotComponent::paintBackground(juce::Graphics& g)
{
}
