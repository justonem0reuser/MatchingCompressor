#pragma once
#define _USE_MATH_DEFINES

#include <math.h>
#include <JuceHeader.h>

class MCLookAndFeel : public juce::LookAndFeel_V4
{
public:
	MCLookAndFeel();
	void drawRotarySlider(
		juce::Graphics& g, 
		int x, 
		int y, 
		int width, 
		int height, 
		float sliderPosProportional, 
		float rotaryStartAngle, float rotaryEndAngle, 
		juce::Slider& slider) override;
	void drawToggleButton(
		juce::Graphics& g,
		juce::ToggleButton& toggleButton,
		bool shouldDrawButtonAsHighlighted,
		bool shouldDrawButtonAsDown) override;
	void fillResizableWindowBackground(
		juce::Graphics& g, 
		int w, 
		int h, 
		const juce::BorderSize<int>& size, 
		juce:: ResizableWindow& window) override;
	juce::Rectangle<int> getPropertyComponentContentPosition(
		juce::PropertyComponent& component) override;

private:
	const int defaultPropertyComponentLabelWidth = 250;

	juce::Image sliderImage, toggleOffImage, toggleOnImage, resizableWindowBackground;
};