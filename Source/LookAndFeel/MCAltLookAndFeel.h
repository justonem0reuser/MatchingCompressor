#pragma once
#include "MCLookAndFeel.h"

class MCAltLookAndFeel : public MCLookAndFeel
{
public:
    MCAltLookAndFeel();

    void drawLeftPanelBackground(
        juce::Graphics& g,
        juce::Rectangle<float> bounds) override;
    void drawRightPanelBackground(
        juce::Graphics& g,
        juce::Rectangle<float> bounds) override;

private:
    juce::Colour leftPanelColour, rightPanelColour, matchWindowColour;
};
