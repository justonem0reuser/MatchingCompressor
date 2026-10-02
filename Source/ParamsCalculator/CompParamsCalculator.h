#pragma once

#include <JuceHeader.h>
#include <cstdint>
#include "alglibinternal.h"
#include "interpolation.h"
#include "../DSP/DynamicShaper.h"
#include "EnvelopeHistogram.h"
#include "ReferenceQuantiles.h"
#include "HashEqualStructures.h"

using ChannelAggregationType = DynamicShaper<float>::ChannelAggregationType;

/// <summary>
/// Matching compressor parameters calculation class
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
    /// <param name="destDetectorSamples">destination audio for the envelope detector (nullptr - destSamples)</param>
    /// <param name="isDestSampleKept">destination audio samples mask (should a sample be used for the statistics)</param>
    /// <returns>best match compressing parameters vector</returns>
    std::vector<float> calculateCompressorParameters(
        std::vector<std::vector<float>>& refSamples,
        std::vector<std::vector<float>>& destSamples,
        double destSampleRate,
        juce::ValueTree& properties,
        const std::vector<std::vector<float>>* destDetectorSamples = nullptr,
        const std::vector<bool>* isDestSampleKept = nullptr);

    void prepare(
        std::vector<std::vector<float>>& refSamples,
        std::vector<std::vector<float>>& destSamples,
        double refSampleRate,
        juce::ValueTree& properties,
        const std::vector<std::vector<float>>* destDetectorSamples = nullptr,
        const std::vector<bool>* isDestSampleKept = nullptr);

    void prepareStructure(
        std::vector<std::vector<float>>& refSamples,
        const juce::ValueTree& properties);

    void prepareForFixation(
        std::vector<std::vector<float>>& destSamples,
        double destSampleRate,
        juce::ValueTree& properties,
        const std::vector<std::vector<float>>* destDetectorSamples = nullptr,
        const std::vector<bool>* isDestSampleKept = nullptr);

    void updateBallistics(float attackMs, float releaseMs, float hpfFrequency, bool isSoftNeeded = true);

    void updateEnvSettings(int balFilterTypeInt, int channelAggregationTypeInt, int useHpfInt);

    std::vector<float> solve(); // fits the reference target built by prepare()

    std::vector<float> solve(
        const std::vector<float>& target,
        const std::vector<float>& targetSoft,
        const std::vector<float>* warmStart = nullptr);

    // prepare()+updateBallistics() must be called before it.
    std::vector<float> calculateQuantilesFor(const std::vector<float>& params);

    void captureNominalKneeWidths(const std::vector<float>& params);

    float scoreAgainstReference(const std::vector<float>& params);

    float getLastFitMismatch() const { return lastFitMismatch; }

    static bool isKWeightingUsed(const juce::ValueTree& properties);

    static std::vector<std::vector<float>> applyKWeighting(
        const std::vector<std::vector<float>>& samples,
        double sampleRate);

    static std::vector<bool> calculateGateMask(
        const std::vector<std::vector<float>>& samples,
        double sampleRate);

    static std::vector<std::vector<float>> removeGatedSamples(
        const std::vector<std::vector<float>>& samples,
        const std::vector<bool>& isSampleKept,
        size_t keptSamplesNumber);

protected:
    constexpr static int paramsPerKnee = 3;
    constexpr static int paramsPerKneeDerivedWidth = 2;

    constexpr static int getStride(bool isWidthVariable)
    {
        return isWidthVariable ? paramsPerKnee : paramsPerKneeDerivedWidth;
    }
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
    /// <param name="maxAmp">peak the material was scaled by</param>
    void setInitGuessAndBounds(
        int kneesNumber,
        alglib::real_1d_array& c,
        alglib::real_1d_array& bndl,
        alglib::real_1d_array& bndu,
        bool isWidthVariable,
        float maxAmp);

    /// <summary>
    /// Converting alglib result vector into std::vector form.
    /// </summary>
    /// <param name="c">alglib result vector</param>
    /// <returns>std::vector result</returns>
    static std::vector<float> resArrayToVector(const alglib::real_1d_array& c);

    static float findMaxAmp(const std::vector<std::vector<float>>& samples);
    static float getScale(float maxAmp) { return maxAmp > 0.f ? 1.f / maxAmp : 1.f; }
    static float getThresholdOffsetDb(float maxAmp)
    {
        return (maxAmp <= 0.f || maxAmp == 1.f) ? 0.f : 20.f * std::log10(maxAmp);
    }
    static void scaleSamples(
        const std::vector<std::vector<float>>& samples,
        std::vector<std::vector<float>>& scaled,
        float scale);

    static void denormalize(std::vector<float>& result, float maxAmp);

private:
    struct FunctionAndJacobian
    {
        std::vector<float> q; // quantiles
        std::vector<std::vector<double>> jac; // [param][quantile]
        bool hasJacobian = false;
    };

    static constexpr double epsx = 0.001;
    static constexpr alglib::ae_int_t maxits = 0;

    static constexpr double gateBlockLengthMs = 400.0;
    static constexpr double gateBlockStepMs = 100.0;
    static constexpr double gatePercentile = 95.0;
    static constexpr double gateRelativeThresholdDb = -30.0;

    std::vector<std::vector<float>> destSamples;
    std::vector<std::vector<float>> destDetectorSamples; // the detector uses destSamples if empty
    std::vector<bool> isDestSampleKept;
    std::size_t keptDestSamplesNumber = 0;
    int gainRegionsNumber, gainRegionsNumberSoft;
    int quantileRegionsNumber, quantileRegionsNumberSoft;
    EnvCalculationType balFilterType;
    ChannelAggregationType channelAggregationType;
    bool isHpfEnabled = false;
    KneeType kneeType = KneeType::hard;
    int kneesNumber = 1;
    double sampleRate = 0.;
    float maxAmp = 1.f;
    ReferenceQuantiles referenceQuantiles, referenceQuantilesSoft;

    // Container for storing and reusing functional calculation results.
    std::unordered_map<alglib::real_1d_array, FunctionAndJacobian, Real1DArrayHash, Real1DArrayEqual> calculatedFunctions;

    DynamicShaper<float> dynamicProcessor;
    juce::dsp::ProcessSpec spec;

    EnvelopeHistogram histogram, histogramSoft;
    const EnvelopeHistogram* activeHistogram = &histogram;

    // Non-empty while the knee widths follow the thresholds instead of being optimized.
    std::vector<double> nominalKneeWidths, kneeWidths, dKneeWidthDThreshold;

    std::vector<double> fixationNominalWidths;

    void updateKneeWidths(const alglib::real_1d_array& c);

    std::vector<double> calculateYDensity(
        const alglib::real_1d_array& params,
        std::vector<std::vector<double>>* dBins = nullptr);

    static void calculateFunctional(
        const alglib::real_1d_array& c,
        const alglib::real_1d_array& x,
        double& func,
        void* ptr);
    static void calculateGradient(
        const alglib::real_1d_array& c,
        const alglib::real_1d_array& x,
        double& func,
        alglib::real_1d_array& grad,
        void* ptr);

    void calculateEnvelopeStatistics(
        std::vector<std::vector<float>>& samples,
        const std::vector<std::vector<float>>& detectorSamples,
        double sampleRate,
        float attackMs,
        float releaseMs,
        float hpfFrequency,
        bool isSoftNeeded);
    std::vector<float> calculateFunction(
        std::vector<std::vector<float>>& samples,
        const alglib::real_1d_array& parameters,
        std::vector<std::vector<double>>* jacobian = nullptr);

    std::vector<float>& getY(const alglib::real_1d_array& c);
    FunctionAndJacobian& getYAndJ(const alglib::real_1d_array& c);

    void paramsToC(const std::vector<float>& params, alglib::real_1d_array& c, bool isWidthVariable);

    void configure(const juce::ValueTree& properties);

    void setDestMask(const std::vector<bool>* isDestSampleKept);

    float fitMismatchAt(const alglib::real_1d_array& c, const std::vector<float>& target);
};