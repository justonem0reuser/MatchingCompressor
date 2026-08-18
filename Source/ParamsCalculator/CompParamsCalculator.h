#pragma once

#include <JuceHeader.h>
#include "alglibinternal.h"

/// <summary>
/// Base matching compressor parameters calculation class
/// </summary>
class CompParamsCalculator
{
public:
    enum class KneeType { hard, soft };

    /// <summary>
    /// Best match compressing parameters calculation.
    /// </summary>
    /// <param name="refSamples">reference audio</param>
    /// <param name="destSamples">destination audio</param>
    /// <param name="destSampleRate">destination sample rate</param>
    /// <param name="properties">calculation properties</param>
    /// <returns>best match compressing parameters vector</returns>
    virtual std::vector<float> calculateCompressorParameters(
        std::vector<std::vector<float>>& refSamples,
        std::vector<std::vector<float>>& destSamples,
        double destSampleRate,
        juce::ValueTree& properties) = 0;

    float getLastFitMismatch() const { return lastFitMismatch; }

protected:
    constexpr static double fineThreshold = 0.1;
    constexpr static double fineCoeff = 100.0;

    constexpr static int paramsPerKnee = 3;

    static float fitMismatch(double rmsError, const std::vector<float>& target);
    float lastFitMismatch = 0.f;


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
    /// <param name="widths">
    /// knee widths taken from here instead of from c, 
    /// for the mode where the width is derived from the thresholds; 
    /// nullptr = read the widths from c
    /// </param>
    /// <returns>fine value</returns>
    static double calculateFine(
        const alglib::real_1d_array& c,
        alglib::real_1d_array* gradPtr = nullptr,
        const double* widths = nullptr);

    /// <summary>
    /// Knee widths derived from their nominal values and the thresholds.
    /// </summary>
    /// <param name="c">array of compression parameters</param>
    /// <param name="kneesNumber">number of compressor knees</param>
    /// <param name="nominalWidths">requested width per knee; the result never exceeds it</param>
    /// <param name="widths">out: resulting width per knee</param>
    /// <param name="dWidthDThreshold">out, optional: derivative of each width over its own threshold</param>
    static void calculateKneeWidths(
        const double* c,
        int kneesNumber,
        const double* nominalWidths,
        double* widths,
        double* dWidthDThreshold = nullptr);

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
    /// Converting alglib result vector into std::vector form.
    /// </summary>
    /// <param name="c">alglib result vector</param>
    /// <returns>std::vector result</returns>
    static std::vector<float> resArrayToVector(const alglib::real_1d_array& c);

    // required for audio data taken from buses as it can exceed [-1; 1] boundaries
    static float normalize(
        const std::vector<std::vector<float>>& refSamples,
        const std::vector<std::vector<float>>& destSamples,
        std::vector<std::vector<float>>& refNormalized,
        std::vector<std::vector<float>>& destNormalized);

    static void denormalize(std::vector<float>& result, float maxAmp);
};