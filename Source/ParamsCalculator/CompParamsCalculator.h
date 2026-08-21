#pragma once

#include <JuceHeader.h>
#include "alglibinternal.h"
#include "interpolation.h"

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
    constexpr static int paramsPerKnee = 3;
    constexpr static int paramsPerKneeDerivedWidth = 2;

    constexpr static int getStride(bool isWidthVariable)
        { return isWidthVariable ? paramsPerKnee : paramsPerKneeDerivedWidth; }
    constexpr static int getThresholdIndex(int kneeIndex, int stride) { return 1 + stride * kneeIndex; }
    constexpr static int getRatioInverseIndex(int kneeIndex, int stride) { return 2 + stride * kneeIndex; }
    constexpr static int getKneeWidthIndex(int kneeIndex) { return 3 + paramsPerKnee * kneeIndex; }
    constexpr static int getKneesNumber(int length, int stride) { return (length - 1) / stride; }
    constexpr static int getVectorLength(int kneesNumber, int stride) { return stride * kneesNumber + 1; }

    static float fitMismatch(double rmsError, const std::vector<float>& target);
    float lastFitMismatch = 0.f;


    static void setKneeConstraints(
        alglib::lsfitstate& state,
        int kneesNumber,
        bool isWidthVariable);

    static void enforceKneeGaps(
        alglib::real_1d_array& c,
        int kneesNumber,
        bool isWidthVariable);

    static void insertKneeWidths(
        alglib::real_1d_array& c,
        int kneesNumber,
        const double* widths);

    static void calculateKneeWidths(
        const double* c,
        int kneesNumber,
        const double* nominalWidths,
        double* widths,
        double* dWidthDThreshold = nullptr);

    /// <summary>
    /// Initial parameters preparation for alglib problem solver.
    /// </summary>
    /// <param name="kneesNumber">number of compressor knees</param>
    /// <param name="c">initial parameter guess</param>
    /// <param name="bndl">parameter left bounds</param>
    /// <param name="bndu">parameter right bounds</param>
    /// <param name="isWidthVariable">
    /// true: optimal widths are defined by the solver;
    /// false: widths are considered nominal unless it violates over-crossing condition
    /// </param>
    void setInitGuessAndBounds(
        int kneesNumber,
        alglib::real_1d_array& c,
        alglib::real_1d_array& bndl,
        alglib::real_1d_array& bndu,
        bool isWidthVariable);

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