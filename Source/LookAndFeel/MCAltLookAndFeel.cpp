#include "MCAltLookAndFeel.h"

MCAltLookAndFeel::MCAltLookAndFeel()
{
    sliderImage      = juce::ImageCache::getFromMemory(BinaryData::slider_minimal_png, BinaryData::slider_minimal_pngSize);
    toggleOffImage   = juce::ImageCache::getFromMemory(BinaryData::toggleoff_minimal_png, BinaryData::toggleoff_minimal_pngSize);
    toggleOnImage    = juce::ImageCache::getFromMemory(BinaryData::toggleon_minimal_png, BinaryData::toggleon_minimal_pngSize);
    toolImage        = juce::ImageCache::getFromMemory(BinaryData::tool_minimal_png, BinaryData::tool_minimal_pngSize);
    toolHoverImage   = juce::ImageCache::getFromMemory(BinaryData::toolhover_minimal_png, BinaryData::toolhover_minimal_pngSize);

    leftPanelColour = juce::Colour(0xff161616);
    rightPanelColour = juce::Colour(0xff131313);
    
    setColour(juce::PopupMenu::ColourIds::backgroundColourId, juce::Colour(0xff1e1e1e));
    setColour(juce::PropertyComponent::ColourIds::backgroundColourId, juce::Colour(0xff161616));
    setColour(juce::ComboBox::ColourIds::backgroundColourId, juce::Colour(0xff1e1e1e));
    setColour(juce::Slider::ColourIds::trackColourId, juce::Colour(0x341de9b6));
    setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, juce::Colour(0xff111111));
    setColour(juce::Label::textColourId, juce::Colour(0xffe8e8e8));
    setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colour(0xff1e1e1e));
    setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colour(0x341de9b6));
    setColour(juce::ChoicePropertyComponent::ColourIds::backgroundColourId, juce::Colour(0xff1e1e1e));

    setColour(ColourIds::controlBackgroundColourId, juce::Colour(0xff1a1a1a));
    setColour(ColourIds::panelVerticalLineColourId, juce::Colour(0xff252525));
    setColour(ColourIds::groupRectColourId, juce::Colour(0xff404040));
    setColour(ColourIds::calculatedCompCurveColourId, juce::Colour(0xff00b89a));
    setColour(ColourIds::actualCompCurveColourId, juce::Colour(0xff1de9b6));
    setColour(ColourIds::thresholdVerticalLineColourId, juce::Colour(0xff777777));
    setColour(ColourIds::documentWindowBackgroundColourId, juce::Colour(0xff111111));
    setColour(ColourIds::plotGridColourId, juce::Colour(0xff777777));
    setColour(ColourIds::plotBackgroundColourId, rightPanelColour);
}

void MCAltLookAndFeel::drawLeftPanelBackground(
    juce::Graphics& g,
    juce::Rectangle<float> bounds)
{
    g.fillAll(leftPanelColour);
}

void MCAltLookAndFeel::drawRightPanelBackground(
    juce::Graphics& g,
    juce::Rectangle<float> bounds)
{
    g.fillAll(rightPanelColour);
}
