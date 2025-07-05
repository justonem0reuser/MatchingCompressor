#pragma once
#include <JuceHeader.h>
class AudioFileReader
{
public:
	AudioFileReader();
	void readFromFile(
		juce::File file,
		bool excludeZeroSamples,
		std::vector<std::vector<float>>& res,
		double& sampleRate);
private:
	juce::AudioFormatManager formatManager;
};