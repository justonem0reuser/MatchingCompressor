#pragma once

#include <JuceHeader.h>

/// <summary>
/// K-weighting filter of ITU-R BS.1770.
/// Coefficients and state are double for accuracy.
/// </summary>
template <typename SampleType>
class KWeightingFilter
{
public:
    /// <summary>
    /// Recalculates the coefficients for the given sample rate and clears the state.
    /// (only spec.sampleRate is used).
    /// </summary>
    void prepare(const juce::dsp::ProcessSpec& spec);

    void reset();

    inline SampleType processSample(SampleType value) noexcept
    {
        return (SampleType)rlb.processSample(shelf.processSample((double)value));
    }

    /// <summary>
    /// Audio block processing, single channel. 
    /// ProcessContextNonReplacing is the expected form.
    /// </summary>
    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        const auto numSamples = outputBlock.getNumSamples();

        jassert(inputBlock.getNumChannels() == 1 && outputBlock.getNumChannels() == 1);
        jassert(inputBlock.getNumSamples() == numSamples);

        if (context.isBypassed)
        {
            outputBlock.copyFrom(inputBlock);
            return;
        }

        auto* inputSamples = inputBlock.getChannelPointer(0);
        auto* outputSamples = outputBlock.getChannelPointer(0);

        for (size_t i = 0; i < numSamples; i++)
            outputSamples[i] = processSample(inputSamples[i]);
    }

private:
    juce::dsp::IIR::Filter<double> shelf, rlb;
};
