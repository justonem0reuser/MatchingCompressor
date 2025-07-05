#include "CompParamsCalculator.h"
#include "../Data/Ranges.h"

void CompParamsCalculator::setInitGuessAndBounds(
    int kneesNumber,
    KneeType kneeType,
    alglib::real_1d_array& c,
    alglib::real_1d_array& bndl, 
    alglib::real_1d_array& bndu)
{
    // c: Gain, [Threshold, 1/Ratio, Knee weight] * kneesNum

    int cLength = 3 * kneesNumber + 1;
    c.setlength(cLength);
    bndl.setlength(cLength);
    bndu.setlength(cLength);

    bndl[0] = gainRange.start;
    bndu[0] = gainRange.end;
    c[0] = 0.;

    for (int i = 0; i < kneesNumber; i++)
    {
        bndl[1 + 3 * i] = thresholdRange.start;
        bndu[1 + 3 * i] = thresholdRange.end;
        c[1 + 3 * i] = thresholdRange.start +
            (thresholdRange.end - thresholdRange.start) * 
            (i + 1) / (kneesNumber + 1);
        bndl[2 + 3 * i] = 1. / ratioRange.end;
        bndu[2 + 3 * i] = 2. - ratioRange.start;
        c[2 + 3 * i] = 1.;
        if (kneeType == KneeType::hard)
            bndl[3 + 3 * i] = bndu[3 + 3 * i] = c[3 + 3 * i] = 0;
        else
        {
            bndl[3 + 3 * i] = kneeWidthRange.start;
            bndu[3 + 3 * i] = kneeWidthRange.end;
            c[3 + 3 * i] = 0.5 * (kneeWidthRange.start + kneeWidthRange.end);
        }
    }
}

std::vector<float> CompParamsCalculator::resArrayToVector(alglib::real_1d_array& c)
{
    auto cLength = c.length();
    auto kneesNumber = (cLength - 1) / 3;
    std::vector<float> res;
    res.resize(cLength);
    res[0] = c[0];
    for (auto i = 0; i < kneesNumber; i++)
    {
        res[3 * i + 1] = c[3 * i + 1];
        res[3 * i + 2] = 1. / c[3 * i + 2];
        res[3 * i + 3] = c[3 * i + 3];
    }
    return res;
}
