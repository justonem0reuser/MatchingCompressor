#include "MCLookAndFeel.h"
#include "Components/SliderWithAttachment.h"
#include "Components/ToggleButtonWithAttachment.h"
#include "Colours.h"

MCLookAndFeel::MCLookAndFeel():
    sliderImage(juce::ImageCache::getFromMemory(
        BinaryData::slider_png,
        BinaryData::slider_pngSize)),
    toggleOffImage(juce::ImageCache::getFromMemory(
        BinaryData::toggleoff_png,
        BinaryData::toggleoff_pngSize)),
    toggleOnImage(juce::ImageCache::getFromMemory(
        BinaryData::toggleon_png,
        BinaryData::toggleon_pngSize)),
    resizableWindowBackground(juce::ImageCache::getFromMemory(
        BinaryData::plotBackground_jpg,
        BinaryData::plotBackground_jpgSize))
{
    setColour(juce::PopupMenu::ColourIds::backgroundColourId, popupMenuBackgroundColour);
    setColour(juce::PropertyComponent::ColourIds::backgroundColourId, textBackgroundColour);
    setColour(juce::ComboBox::ColourIds::backgroundColourId, textBackgroundColour);
    setColour(juce::Slider::ColourIds::trackColourId, buttonColour);
    setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, juce::Colour(linearSliderTextboxBackgroundColour)); // rotary slider color is set inside an appropriate method
    setColour(juce::Label::textColourId, textColour);
    setColour(juce::TextButton::ColourIds::buttonColourId, buttonColour);
    setColour(juce::TextButton::ColourIds::buttonOnColourId, buttonOnColour);
    setColour(juce::ChoicePropertyComponent::ColourIds::backgroundColourId, textBackgroundColour);
}

void MCLookAndFeel::drawRotarySlider(
    juce::Graphics& g, 
    int x, 
    int y, 
    int width, 
    int height, 
    float sliderPosProportional, 
    float rotaryStartAngle, 
    float rotaryEndAngle, 
    juce::Slider& slider)
{
    const int sliderImageWidth = sliderImage.getWidth();
    const int numFrames = sliderImage.getHeight() / sliderImageWidth;
    const float modAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const int frameIdx = (int)std::ceil(modAngle * 0.5f / M_PI * (numFrames - 1)) % numFrames;

    int diameter, rx, ry;
    if (width > height)
    {
        diameter = height;
        rx = x + (width - height) / 2;
        ry = y + 10;
    }
    else
    {
        diameter = width;
        rx = x;
        ry = y + 10 + (height - width) / 2;
    }

    // another color is set for linear sliders
    slider.setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, controlBackgroundColour);
    g.drawImage(
        sliderImage,
        rx,
        ry,
        diameter,
        diameter,
        0,
        frameIdx * sliderImageWidth,
        sliderImageWidth,
        sliderImageWidth);
    g.setColour(textColour);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText(slider.getName(), x, y, width, 15, juce::Justification::centred, false);
}

void MCLookAndFeel::drawToggleButton(
    juce::Graphics& g, 
    juce::ToggleButton& toggleButton, 
    bool shouldDrawButtonAsHighlighted, 
    bool shouldDrawButtonAsDown)
{
    if (auto* buttonWithAtt = dynamic_cast<ToggleButtonWithAttachment*>(&toggleButton))
    {
        const juce::Image& image = 
            toggleButton.getToggleState() ? toggleOffImage : toggleOnImage;
        auto size = std::min(toggleButton.getWidth(), toggleButton.getHeight());
        g.drawImage(image, 0, 0, size, size, 0, 0, image.getWidth(), image.getHeight());
    }
    else
    {
        juce::LookAndFeel_V4::drawToggleButton(g, toggleButton, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }
}

void MCLookAndFeel::fillResizableWindowBackground(
    juce::Graphics& g, 
    int w, 
    int h, 
    const juce::BorderSize<int>& size, 
    juce::ResizableWindow& window)
{
    if (dynamic_cast<juce::DocumentWindow*>(&window))
        g.drawImage(resizableWindowBackground, window.getLocalBounds().toFloat());
}

juce::Rectangle<int> MCLookAndFeel::getPropertyComponentContentPosition(juce::PropertyComponent& component)
{
    auto halfWidth = component.getWidth() / 2;
    return { halfWidth, 0, halfWidth, component.getHeight() - 1 };
}
