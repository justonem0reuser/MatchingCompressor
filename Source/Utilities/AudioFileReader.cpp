#include "AudioFileReader.h"

AudioFileReader::AudioFileReader()
{
	formatManager.registerBasicFormats();
}

void AudioFileReader::readFromFile(
    juce::File file,
    bool excludeZeroSamples,
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
                juce::AudioBuffer<float> buffer(numChannels, length);
                buffer.clear();
                if (reader->read(buffer.getArrayOfWritePointers(), numChannels, 0, length))
                {
                    res.clear();
                    for (int i = 0; i < numChannels; i++)
                    {
                        res.push_back(std::vector<float>{});
                        res[i].reserve(length);
                    }
                    if (excludeZeroSamples)
                        for (int i = 0; i < length; i++)
                        {
                            bool isZero = true;
                            for (int j = 0; j < numChannels; j++)
                                isZero &= buffer.getSample(j, i) == 0;
                            if (!isZero)
                                for (int j = 0; j < numChannels; j++)
                                    res[j].push_back(buffer.getSample(j, i));
                        }
                    else
                        for (int j = 0; j < numChannels; j++)
                        {
                            auto* readPointer = buffer.getReadPointer(j);
                            res[j].insert(res[j].begin(), readPointer, readPointer + length);
                        }
                }
            }
        }
    }
}