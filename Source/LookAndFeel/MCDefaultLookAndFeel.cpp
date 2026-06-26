#include "MCDefaultLookAndFeel.h"

MCDefaultLookAndFeel::MCDefaultLookAndFeel()
{
    sliderImage      = juce::ImageCache::getFromMemory(BinaryData::slider_brutal_png, BinaryData::slider_brutal_pngSize);
    toggleOffImage   = juce::ImageCache::getFromMemory(BinaryData::toggleoff_brutal_png, BinaryData::toggleoff_brutal_pngSize);
    toggleOnImage    = juce::ImageCache::getFromMemory(BinaryData::toggleon_brutal_png, BinaryData::toggleon_brutal_pngSize);
    toolImage        = juce::ImageCache::getFromMemory(BinaryData::tool_brutal_png, BinaryData::tool_brutal_pngSize);
    toolHoverImage   = juce::ImageCache::getFromMemory(BinaryData::toolhover_brutal_png, BinaryData::toolhover_brutal_pngSize);
    leftPanelImage   = juce::ImageCache::getFromMemory(BinaryData::background_brutal_jpg, BinaryData::background_brutal_jpgSize);
    rightPanelImage  = juce::ImageCache::getFromMemory(BinaryData::plotBackground_brutal_jpg, BinaryData::plotBackground_brutal_jpgSize);
    matchWindowImage = rightPanelImage;

    setColour(juce::PopupMenu::ColourIds::backgroundColourId, juce::Colour(0xff574f4f));
    setColour(juce::PropertyComponent::ColourIds::backgroundColourId, juce::Colour(0x408b7460));
    setColour(juce::ComboBox::ColourIds::backgroundColourId, juce::Colour(0x408b7460));
    setColour(juce::Slider::ColourIds::trackColourId, juce::Colour(0x80392f27));
    setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, juce::Colour(0x40332820));
    setColour(juce::Label::textColourId, juce::Colours::white);
    setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colour(0x80392f27));
    setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colour(0x50ff7100));
    setColour(juce::ChoicePropertyComponent::ColourIds::backgroundColourId, juce::Colour(0x408b7460));

    setColour(ColourIds::controlBackgroundColourId, juce::Colour(0x86574f4f));
    setColour(ColourIds::panelVerticalLineColourId, juce::Colour(0x95785b57));
    setColour(ColourIds::groupRectColourId, juce::Colour(0x80ffffff));
    setColour(ColourIds::calculatedCompCurveColourId, juce::Colours::white);
    setColour(ColourIds::actualCompCurveColourId, juce::Colour(0xff49d119));
    setColour(ColourIds::thresholdVerticalLineColourId, juce::Colours::lightgrey);
    setColour(ColourIds::documentWindowBackgroundColourId, juce::Colours::brown);
    setColour(ColourIds::plotGridColourId, juce::Colours::white);
    setColour(ColourIds::plotBackgroundColourId, juce::Colours::black);
}

void MCDefaultLookAndFeel::fillResizableWindowBackground(
    juce::Graphics& g,
    int w,
    int h,
    const juce::BorderSize<int>& size,
    juce::ResizableWindow& window)
{
    if (dynamic_cast<juce::DocumentWindow*>(&window))
        g.drawImage(matchWindowImage, window.getLocalBounds().toFloat());
}

void MCDefaultLookAndFeel::drawLeftPanelBackground(
    juce::Graphics& g,
    juce::Rectangle<float> bounds)
{
    auto imageBounds = bounds;
    imageBounds.setWidth(
        leftPanelImage.getWidth() * bounds.getHeight() / (float)leftPanelImage.getHeight());
    g.drawImage(leftPanelImage, imageBounds);
}

void MCDefaultLookAndFeel::drawRightPanelBackground(
    juce::Graphics& g,
    juce::Rectangle<float> bounds)
{
    g.drawImage(rightPanelImage, bounds);
    g.setColour(findColour(controlBackgroundColourId));
    g.fillRect(bounds);
}

