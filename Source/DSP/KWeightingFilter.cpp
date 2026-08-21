#include "KWeightingFilter.h"

namespace
{
    constexpr double tableSampleRate = 48000.0;

    // The shelving stage modelling the acoustic effect of the head.
    constexpr double shelfB0 = 1.53512485958697;
    constexpr double shelfB1 = -2.69169618940638;
    constexpr double shelfB2 = 1.19839281085285;
    constexpr double shelfA1 = -1.69065929318241;
    constexpr double shelfA2 = 0.73248077421585;

    // The RLB stage (only denominator; the numerator is (1, -2, 1).
    constexpr double rlbA1 = -1.99004745483398;
    constexpr double rlbA2 = 0.99007225036621;

    /// Shelving stage for the given sample rate.
    juce::dsp::IIR::Coefficients<double>::Ptr makeShelf(double sampleRate)
    {
        auto gain = (shelfB0 - shelfB1 + shelfB2) / (1.0 - shelfA1 + shelfA2);
        auto a = std::sqrt(gain);
        auto ratio = shelfA1 / (1.0 + shelfA2);
        auto cosW = ((a - 1.0) - ratio * (a + 1.0)) / ((a + 1.0) - ratio * (a - 1.0));
        auto w = std::acos(cosW);
        auto beta = (1.0 - shelfA2) / (1.0 + shelfA2) * ((a + 1.0) - (a - 1.0) * cosW);

        return juce::dsp::IIR::Coefficients<double>::makeHighShelf(
            sampleRate,
            w * tableSampleRate / juce::MathConstants<double>::twoPi,
            std::sin(w) * std::sqrt(a) / beta,
            gain);
    }

    /// RLB stage for the given sample rate.
    juce::dsp::IIR::Coefficients<double>::Ptr makeRlb(double sampleRate)
    {
        auto sum = 1.0 + rlbA1 + rlbA2;
        auto difference = 1.0 - rlbA1 + rlbA2;
        auto tangent = std::sqrt(sum / difference);

        auto highPass = juce::dsp::IIR::Coefficients<double>::makeHighPass(
            sampleRate,
            tableSampleRate * std::atan(tangent) / juce::MathConstants<double>::pi,
            tangent * difference / (2.0 * (1.0 - rlbA2)));

        auto* c = highPass->getRawCoefficients();
        auto scale = 1.0 / c[0];
        return new juce::dsp::IIR::Coefficients<double>(
            c[0] * scale, c[1] * scale, c[2] * scale,
            1.0, c[3], c[4]);
    }
}

template <typename SampleType>
void KWeightingFilter<SampleType>::prepare(const juce::dsp::ProcessSpec& spec)
{
    jassert(spec.sampleRate > 0);

    shelf.coefficients = makeShelf(spec.sampleRate);
    rlb.coefficients = makeRlb(spec.sampleRate);
    reset();
}

template <typename SampleType>
void KWeightingFilter<SampleType>::reset()
{
    shelf.reset();
    rlb.reset();
}

//==============================================================================
template class KWeightingFilter<float>;
template class KWeightingFilter<double>;
