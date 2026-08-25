#pragma once
#include <JuceHeader.h>
#include <cstdint>
#include "../DSP/DynamicShaper.h"

class EnvelopeHistogram
{
public:
    void prepare(int newSide)
    {
        side = newSide;
        table.assign((size_t)(side * side), 0);
        envDbByCol.resize(side);
        const double delta = 1.0 / side;
        for (int i = 0; i < side; i++)
            envDbByCol[i] = juce::Decibels::gainToDecibels(
                (i + 0.5) * delta, DynamicShaper<double>::minusInfinityDb);
    }

    void add(float amplitude, float env)
    {
        const int i = std::min((int)(amplitude * side), side - 1);
        const int j = std::min((int)(env * side), side - 1);
        table[i * side + j]++;
    }

    int getSide() const { return side; }

    const std::int32_t* getRow(int rowIndex) const { return table.data() + rowIndex * side; }

    double getEnvDb(int columnIndex) const { return envDbByCol[columnIndex]; }

private:
    int side = 0;
    std::vector<std::int32_t> table;
    std::vector<double> envDbByCol;
};
