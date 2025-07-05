#include "CurveCalculator.h"

CurveCalculator::CurveCalculator(CurveBounds& curveBounds) :
    curveBounds(curveBounds)
{
}

juce::Path CurveCalculator::calculateCurve(
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
            float startThreshold = std::min(curveBounds.inputXMin, kneeStart);
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
            float dx = -curveBounds.inputXMin / (curveBounds.graphXMax - curveBounds.graphXMin);
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
        float rightBoundX = i == size - 1 ? curveBounds.inputXMax :
            compParams[3 * i + 4] - 0.5f * compParams[3 * i + 6];
        float rightBoundY = compThreshold + (rightBoundX - compThreshold) * compRatioM1 + compGain;
        curve.lineTo(mapX(rightBoundX), mapY(rightBoundY));
        lastCompRatioM1 = compRatioM1;
    }

    return curve;
}

float CurveCalculator::mapX(float x) const
{
    return juce::jmap(x, curveBounds.inputXMin, curveBounds.inputXMax, curveBounds.graphXMin, curveBounds.graphXMax);
}

float CurveCalculator::mapY(float y) const
{
    return juce::jmap(y, curveBounds.inputYMin, curveBounds.inputYMax, curveBounds.graphYMin, curveBounds.graphYMax);
}

