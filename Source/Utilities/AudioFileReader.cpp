#include "AudioFileReader.h"

AudioFileReader::AudioFileReader()
{
	formatManager.registerBasicFormats();
}

void AudioFileReader::readFromFile(
    juce::File file,
    std::vector<std::vector<float>>& res,
    double& sampleRate)
{
    sampleRate = 0.0;

    if (file != juce::File{})
    {
        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(file));
        if (reader != nullptr)
        {
            auto length = reader->lengthInSamples;
            auto numChannels = reader->numChannels;
            sampleRate = reader->sampleRate;
            if (length > 0 && numChannels > 0 && sampleRate > 0)
            {
                std::vector<std::vector<float>> samples(numChannels, std::vector<float>(length));
                std::vector<float*> writePointers;
                for (auto& channel : samples)
                    writePointers.push_back(channel.data());
                if (reader->read(writePointers.data(), numChannels, 0, length))
                    res = std::move(samples);
            }
        }
    }
}