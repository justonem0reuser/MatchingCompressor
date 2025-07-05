#pragma once
#include <JuceHeader.h>
struct ParameterInfo
{
public:
	ParameterInfo(juce::String name,
		juce::String text,
		juce::var defaultValue,
		juce::var minValue,
		juce::var maxValue,
		juce::var step,
		bool isBoolean,
		bool isInteger) :
		name(name),
		text(text),
		defaultValue(defaultValue),
		minValue(minValue),
		maxValue(maxValue),
		step(step),
		isBoolean(isBoolean),
		isInteger(isInteger),
		isTextChoice(false),
		choices()
	{}

	ParameterInfo(juce::String name,
		juce::String text,
		juce::var defaultValueIndex,
		juce::StringArray choices) :
		name(name),
		text(text),
		defaultValue(defaultValueIndex),
		minValue(1),
		maxValue(choices.size()),
		step(1),
		isBoolean(false),
		isInteger(false),
		isTextChoice(true),
		choices(choices)
	{}

	juce::String name;
	juce::String text;
	juce::var defaultValue;
	juce::var minValue;
	juce::var maxValue;
	juce::var step;
	juce::StringArray choices;
	bool isBoolean;
	bool isInteger;
	bool isTextChoice;
};