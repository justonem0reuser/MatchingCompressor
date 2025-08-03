#pragma once

#include <JuceHeader.h>
#include "alglibinternal.h"

/// <summary>
/// Base matching compressor parameters calculation class
/// </summary>
class CompParamsCalculator
{
public:
    enum KneeType
    {
        hard,
        soft
    };

    /// <summary>
    /// Best match compressing parameters calculation.
    /// </summary>
    /// <param name="refSamples">reference audio</param>
    /// <param name="refSampleRate">reference sample rate</param>
    /// <param name="destSamples">destination audio</param>
    /// <param name="destSampleRate">destination sample rate</param>
    /// <param name="properties">calculation properties</param>
    /// <returns>best match compressing parameters vector</returns>
    virtual std::vector<float> calculateCompressorParameters(
        std::vector<std::vector<float>>& refSamples, 
        double refSampleRate,
        std::vector<std::vector<float>>& destSamples, 
        double destSampleRate,
        juce::ValueTree& properties) = 0;

protected:

    /// <summary>
    /// Fine calculation in case 
    /// if the left bound of the previous knee is greater 
    /// than the right bound of the next knee
    /// or close to it.
    /// Fine should be great enough to avoid such decisions.
    /// </summary>
    /// <param name="c">
    /// alglib array of compression parameters
    /// </param>
    /// <param name="gradPtr">
    /// gradient array
    /// (if it is not nullptr then fine coefficients are added to it)
    /// </param>
    /// <returns>fine value</returns>
    static double calculateFine(
        const alglib::real_1d_array& c,
        alglib::real_1d_array* gradPtr = nullptr);

    /// <summary>
    /// Initial parameters preparation for 
    /// alglib problem solver.
    /// </summary>
    /// <param name="kneesNumber">number of compressor knees</param>
    /// <param name="kneeType">knee type (soft or hard)</param>
    /// <param name="c">initial parameter guess</param>
    /// <param name="bndl">parameter left bounds</param>
    /// <param name="bndu">parameter right bounds</param>
    void setInitGuessAndBounds(
        int kneesNumber,
        KneeType kneeType,
        alglib::real_1d_array& c,
        alglib::real_1d_array& bndl,
        alglib::real_1d_array& bndu);

    /// <summary>
    /// Converting alglib result vector into std::vector form
    /// with converting 1/ratio to ratio.
    /// </summary>
    /// <param name="c">alglib result vector</param>
    /// <returns>std::vector result</returns>
    std::vector<float> resArrayToVector(alglib::real_1d_array& c);

private:
    constexpr static double fineThreshold = 0.1;
    constexpr static double fineCoeff = 100.0;
};