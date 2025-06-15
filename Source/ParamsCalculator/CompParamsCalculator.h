#pragma once

#include <JuceHeader.h>
#include "alglibinternal.h"

class CompParamsCalculator
{
public:
    enum KneeType
    {
        hard,
        soft
    };

    virtual std::vector<float> calculateCompressorParameters(
        std::vector<std::vector<float>>& refSamples, 
        double refSampleRate,
        std::vector<std::vector<float>>& destSamples, 
        double destSampleRate,
        juce::ValueTree& properties) = 0;

protected:
    void setInitGuessAndBounds(
        int kneesNumber,
        KneeType kneeType,
        alglib::real_1d_array& c,
        alglib::real_1d_array& bndl,
        alglib::real_1d_array& bndu);

    std::vector<float> resArrayToVector(alglib::real_1d_array& c);
};