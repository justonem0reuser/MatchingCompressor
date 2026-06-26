#include "MCLookAndFeel.h"
#include "../Components/ToggleButtonWithAttachment.h"

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
    const int frameIdx = (int)std::ceil(modAngle * 0.5f / pi * (numFrames - 1)) % numFrames;

    int diameter, rx, ry;
    if (width > height)
    {
        diameter = height - sliderReducing;
        rx = x + (width - height) / 2;
        ry = y + sliderTopMargin;
    }
    else
    {
        diameter = width - sliderReducing;
        rx = x;
        ry = y + sliderTopMargin + (height - width) / 2;
    }

    // another color is set for linear sliders
    slider.setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, findColour(controlBackgroundColourId));
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
    g.setColour(findColour(juce::Label::textColourId));
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

juce::Rectangle<int> MCLookAndFeel::getPropertyComponentContentPosition(juce::PropertyComponent& component)
{
    auto halfWidth = component.getWidth() / 2;
    return { halfWidth, 0, halfWidth, component.getHeight() - 1 };
}

void MCLookAndFeel::setupToolButton(juce::ImageButton& button)
{
    // resizeButtonNowToFitThisImage must stay false 
    // so re-applying a theme does not resize the button to the image's native size.
    button.setImages(false, true, true,
        toolImage, 1.0f, juce::Colours::transparentWhite,
        toolHoverImage, 1.0f, juce::Colours::transparentWhite,
        toolHoverImage, 1.0f, juce::Colours::transparentWhite);
}

void MCLookAndFeel::setupKneeIndexButton(juce::ImageButton& button)
{
    button.setImages(false, true, true,
        toggleOffImage, 1.0f, juce::Colours::transparentWhite,
        toggleOffImage, 1.0f, juce::Colours::transparentWhite,
        toggleOnImage, 1.0f, juce::Colours::transparentWhite);
    button.setClickingTogglesState(true);
}
