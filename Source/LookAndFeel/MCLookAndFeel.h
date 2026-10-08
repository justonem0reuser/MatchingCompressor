#pragma once

#include <JuceHeader.h>

class MCLookAndFeel : public juce::LookAndFeel_V4
{
public:
    enum ColourIds
    {
        controlBackgroundColourId = 0x2000001,
        panelVerticalLineColourId,
        groupRectColourId,
        calculatedCompCurveColourId,
        actualCompCurveColourId,
        thresholdVerticalLineColourId,
        documentWindowBackgroundColourId,
        plotGridColourId,
        plotBackgroundColourId
    };

    void drawRotarySlider(
        juce::Graphics& g,
        int x,
        int y,
        int width,
        int height,
        float sliderPosProportional,
        float rotaryStartAngle, float rotaryEndAngle,
        juce::Slider& slider) override;
    void drawButtonBackground(
        juce::Graphics& g,
        juce::Button& button,
        const juce::Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;
    juce::Font getTextButtonFont(
        juce::TextButton& button,
        int buttonHeight) override;
    juce::Rectangle<int> getPropertyComponentContentPosition(
        juce::PropertyComponent& component) override;

    virtual void drawLeftPanelBackground(
        juce::Graphics& g,
        juce::Rectangle<float> bounds) = 0;
    virtual void drawRightPanelBackground(
        juce::Graphics& g,
        juce::Rectangle<float> bounds) = 0;

    void setupToolButton(juce::ImageButton& button);
    void setupKneeIndexButton(juce::ImageButton& button);

protected:
    const int defaultPropertyComponentLabelWidth = 250;
    const int sliderReducing = 12;
    const int sliderTopMargin = 15;

    static constexpr double pi = juce::MathConstants<double>::pi;

    juce::Image sliderImage, toggleOffImage, toggleOnImage,
        toolImage, toolHoverImage;
};
