#ifndef POW10_H
#define POW10_H

#include <limits>

namespace parsimon::internal {

template <typename Floating>
struct pow_table_instance {
    static constexpr int max = std::numeric_limits<Floating>::max_exponent10;
    static constexpr int min = std::numeric_limits<Floating>::min_exponent10;
    static constexpr int maxt = std::numeric_limits<Floating>::max_exponent;
    static constexpr int size = max - min + 1;
    Floating table[size] = {};
    constexpr auto fill() {
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
    }
    constexpr pow_table_instance() : table() {fill();}

    constexpr auto pow(std::size_t n) const {
        return table[-min + n];
    }
};

template <typename Floating>
struct pow_table {
    static constexpr pow_table_instance<Floating> table{};
    static constexpr auto pow(std::size_t n) {
        return table.pow(n);
    }
};
}

#endif // POW10_H
