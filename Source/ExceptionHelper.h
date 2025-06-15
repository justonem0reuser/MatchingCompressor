#pragma once
#include <JuceHeader.h>

static class ExceptionHelper
{
public:
	static void catchException(const std::exception& e, juce::Component* parent)
	{
		juce::NativeMessageBox::showMessageBoxAsync(
			juce::MessageBoxIconType::WarningIcon,
			"Error", 
			e.what(), 
			parent);
	}
};