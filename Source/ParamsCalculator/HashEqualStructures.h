#pragma once

/// <summary>
/// Structure for calculating the inner std::unordered_map key from alglib array
/// </summary>
struct Real1DArrayHash {
    size_t operator()(const alglib::real_1d_array& v) const
    {
        auto length = v.length();
        size_t seed = length;
        std::hash<double> hasher;
        for (auto i = 1; i < length; i++) // v[0] is gain and is not taken into account
            seed ^= hasher(v[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

/// <summary>
/// Structure for comparing std::unordered_map keys that are alglib arrays
/// </summary>
struct Real1DArrayEqual {
    bool operator()(const alglib::real_1d_array& lhs, const alglib::real_1d_array& rhs) const
    {
        auto length = lhs.length();
        if (length != rhs.length())
            return false;
        for (auto i = 1; i < length; i++) // v[0] is gain and is not taken into account
            if (lhs[i] != rhs[i]) 
                return false;
        return true;
    }
};
