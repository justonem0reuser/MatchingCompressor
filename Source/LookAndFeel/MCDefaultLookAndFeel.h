#pragma once
#include "MCLookAndFeel.h"

class MCDefaultLookAndFeel : public MCLookAndFeel
{
public:
    MCDefaultLookAndFeel();

    void drawLeftPanelBackground(
        juce::Graphics& g,
        juce::Rectangle<float> bounds) override;
    void drawRightPanelBackground(
        juce::Graphics& g,
        juce::Rectangle<float> bounds) override;
    void fillResizableWindowBackground(
        juce::Graphics& g,
        int w,
        int h,
        const juce::BorderSize<int>& size,
        juce::ResizableWindow& window) override;

private:
    juce::Image leftPanelImage, rightPanelImage, matchWindowImage;
};
