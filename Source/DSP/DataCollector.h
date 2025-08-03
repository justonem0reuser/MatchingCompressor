#pragma once

#include <JuceHeader.h>

/// <summary>
/// Collects input data from bus in real-time
/// for saving it in memory
/// </summary>
template <typename SampleType>
class DataCollector
{
public:
	std::function<void()> onMemoryFull;

	/// <summary>
	/// Audio block processing
	/// (called from AudioProcessor processBlock method).
	/// </summary>
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
	
	/// <summary>
	/// Audio block processing preparation
	/// (called from AudioProcessor prepareToPlay method).
	/// </summary>
	void prepare(const juce::dsp::ProcessSpec& spec);
	
	void reset() noexcept;

	/// <summary>
	/// Saves the collected data to the parameters
	/// </summary>
	/// <param name="data">audio data</param>
	/// <param name="rate">sample rate</param>
	/// <remarks>
	/// For optimizing purposes, data is checking before saving:
	/// if both channels are equal or one channel contains only silence
	/// then audio is considered as mono and only one channel is kept.
	/// </remarks>
	void getData(std::vector<std::vector<SampleType>>& data, double& rate);
	
	/// <summary>
	/// Sets start input channel
	/// </summary>
	/// <param name="startChannelNumber">
	/// 0 for main bus; > 0 for sidechain
	/// </param>
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