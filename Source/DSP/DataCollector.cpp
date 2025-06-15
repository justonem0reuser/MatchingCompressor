#include "DataCollector.h"

template<typename Sampletype>
void DataCollector<Sampletype>::prepare(const juce::dsp::ProcessSpec& spec)
{
	jassert(spec.sampleRate > 0);
	sampleRate = spec.sampleRate;
	dataMaxSize = maxTimeSec * sampleRate;
	channelNumber = spec.numChannels;
	data.clear();
	for (int i = 0; i < channelNumber; i++)
	{
		data.push_back({});
		data[i].reserve(dataMaxSize);
	}
}

template<typename Sampletype>
void DataCollector<Sampletype>::reset() noexcept
{
	for (int i = 0; i < channelNumber; i++)
		data[i].resize(0);
	areChannelsEqual = isLeftZero = isRightZero = true;
}

template<typename SampleType>
void DataCollector<SampleType>::getData(
	std::vector<std::vector<SampleType>>& data, 
	double& rate)
{
	rate = sampleRate;
	data = channelNumber < 2 ? this->data : 
		(areChannelsEqual || isRightZero) ? std::vector<std::vector<SampleType>>{ this->data[0] } :
		isLeftZero ? std::vector<std::vector<SampleType>>{ this->data[1] } :
		this->data;
}

template<typename SampleType>
void DataCollector<SampleType>::setStartChannelNumber(int startChannelNumber)
{
	this->startChannelNumber = startChannelNumber;
}

//==============================================================================
template class DataCollector<float>;
template class DataCollector<double>;
