#ifndef PARSIMON_INTERNAL_POW10_H
#define PARSIMON_INTERNAL_POW10_H

#include <limits>
#include <array>

namespace anpa::internal {

template <typename Floating>
struct pow_table {
    static constexpr int min = std::numeric_limits<Floating>::min_exponent10;

    static constexpr auto table = []() {
        constexpr int max = std::numeric_limits<Floating>::max_exponent10;
        constexpr int size = max - min + 1;
        std::array<Floating, size> table{};
        for (int i = 0; i <= max; ++i) {
            if (i == 0) {
                table[-min + i] = 1;
            } else {
                table[-min + i] = table[-min + i-1] * 10;
            }
        }
        for (int i = -1; i >= min; --i) {
            table[-min + i] = table[-min + i + 1] / 10;
        }
        return table;
    }();

    static constexpr auto pow(std::size_t n) {
        return table[-min + n];
    }
};
}

#endif // PARSIMON_INTERNAL_POW10_H
