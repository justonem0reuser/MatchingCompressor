#include "CurvePlotComponent.h"
#include "../Data/Messages.h"

CurvePlotComponent::CurvePlotComponent(
    const juce::Colour gridColour,
    const juce::Colour calculatedCurveColor,
    const juce::Colour actualCurveColor,
    const juce::Colour thresholdVerticalLineColor):
PlotWithCoordinateSystemComponent(
        margin, margin, margin, margin,
        plotMargin, plotMargin, plotMargin, plotMargin,
        "dB", "dB",
        juce::Colours::black, // will not be used
        gridColour),
    calculatedCurveColor(calculatedCurveColor),
    actualCurveColor(actualCurveColor),
    thresholdVerticalLineColor(thresholdVerticalLineColor)
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
        g.setColour(calculatedCurveColor);
        g.strokePath(calculatedCompCurve, juce::PathStrokeType(1.f));

        actualCompCurve = calculateCurve(actualCompParams, kneePoints);
        g.setColour(actualCurveColor);
        g.strokePath(actualCompCurve, juce::PathStrokeType(3.f));

        g.setColour(thresholdVerticalLineColor);
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

    juce::Path curve;
    float compGain, compThreshold, compRatioM1, compKneeWidth, kneeStart, kneeEnd;
    float lastCompRatioM1 = 1.f;
    for (int i = 0; i < size; i++)
    {
        if (compParams[3 * i + 1] > 0.f ||
            compParams[3 * i + 2] <= 0.f ||
            compParams[3 * i + 3] < 0.f)
            return {};
        compThreshold = compParams[3 * i + 1];
        compRatioM1 = 1.f / compParams[3 * i + 2];
        compKneeWidth = compParams[3 * i + 3];
        kneeStart = compThreshold - 0.5f * compKneeWidth;
        kneeEnd = kneeStart + compKneeWidth;

        if (i == 0)
        {
            compGain = compParams[0];
            float startThreshold = std::min(inputXMin, kneeStart);
            curve.startNewSubPath(mapX(startThreshold), mapY(startThreshold + compGain));
            curve.lineTo(mapX(kneeStart), mapY(kneeStart + compGain));
        }
        else
            compGain += (compThreshold - compParams[3 * i - 2]) * (lastCompRatioM1 - 1.f);

        if (compKneeWidth > 0)
        {
            kneePoints.push_back(curve.getCurrentPosition());
            float aQuadCoeff = 0.5f * (compRatioM1 - lastCompRatioM1) / compKneeWidth;
            float bQuadCoeff = lastCompRatioM1 - 2.f * aQuadCoeff * kneeStart;
            float cQuadCoeff = compThreshold -
                0.5f * lastCompRatioM1 * compKneeWidth -
                kneeStart * (kneeStart * aQuadCoeff + bQuadCoeff);

            float x = kneeStart;
            float dx = -inputXMin / (graphXMax - graphXMin);
            if (compParams[3 * i + 2] < 1.f)
                dx *= compParams[3 * i + 2];
            for (x += dx; x < kneeEnd && x < 0.f; x += dx)
            {
                float y = x * (x * aQuadCoeff + bQuadCoeff) + cQuadCoeff + compGain;
                curve.lineTo(mapX(x), mapY(y));
            }
            if (kneeEnd <= 0.f)
            {
                float y = kneeEnd * (kneeEnd * aQuadCoeff + bQuadCoeff) + cQuadCoeff + compGain;
                curve.lineTo(mapX(kneeEnd), mapY(y));
                kneePoints.push_back(curve.getCurrentPosition());
            }
        }
        float rightBoundX = i == size - 1 ? inputXMax :
            compParams[3 * i + 4] - 0.5f * compParams[3 * i + 6];
        float rightBoundY = compThreshold + (rightBoundX - compThreshold) * compRatioM1 + compGain;
        curve.lineTo(mapX(rightBoundX), mapY(rightBoundY));
        lastCompRatioM1 = compRatioM1;
    }

    return curve;
}

void CurvePlotComponent::paintBackground(juce::Graphics& g)
{
}
