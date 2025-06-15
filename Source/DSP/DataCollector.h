#pragma once

#include <JuceHeader.h>

template <typename SampleType>
class DataCollector
{
public:
	std::function<void()> onMemoryFull;

	template <typename ProcessContext>
	void process(const ProcessContext& context) noexcept
	{
		if (context.isBypassed)
			return;
		const auto& inputBlock = context.getInputBlock();
		auto& outputBlock = context.getOutputBlock();
		const auto numSamples = outputBlock.getNumSamples();

		outputBlock.copyFrom(inputBlock); // copies only mainChanels (min channels number)
		if (channelNumber > 0)
		{
			auto size = data[0].size();
			if (channelNumber == 1)
			{
				auto* channelSamples = inputBlock.getChannelPointer(startChannelNumber);
				for (int i = 0; i < numSamples; i++)
				{
					auto sample = channelSamples[i];
					if (sample != 0)
					{
						if (size == dataMaxSize)
						{
							juce::NullCheckedInvocation::invoke(onMemoryFull);
							return;
						}
						isLeftZero = false;
						data[0].push_back(sample);
					}
				}
			}
			else
			{
				auto* channelSamplesLeft = inputBlock.getChannelPointer(startChannelNumber);
				auto* channelSamplesRight = inputBlock.getChannelPointer(startChannelNumber + 1);
				for (int i = 0; i < numSamples; i++)
				{
					auto sampleLeft = channelSamplesLeft[i];
					auto sampleRight = channelSamplesRight[i];
		
					areChannelsEqual &= sampleLeft == sampleRight;
					bool isSampleLeftZero = sampleLeft == 0;
					bool isSampleRightZero = sampleRight == 0;
					if (!(isSampleLeftZero && isSampleRightZero))
					{
						if (size == dataMaxSize) 
						{
							juce::NullCheckedInvocation::invoke(onMemoryFull);
							return;
						}
						isLeftZero &= isSampleLeftZero;
						isRightZero &= isSampleRightZero;
						data[0].push_back(sampleLeft);
						data[1].push_back(sampleRight);
					}
				}
			}
		}
	}
	
	void prepare(const juce::dsp::ProcessSpec& spec);
	void reset() noexcept;

	void getData(std::vector<std::vector<SampleType>>& data, double& rate);
	void setStartChannelNumber(int startChannelNumber);

private:
	const int maxTimeSec = 600;
	
	std::vector<std::vector<SampleType>> data;
	double sampleRate;
	int channelNumber;
	bool isLeftZero, isRightZero;
	bool areChannelsEqual;
	int dataMaxSize;
	int startChannelNumber = 0;
};