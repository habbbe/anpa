#ifndef PARSIMON_OPTIONS_H
#define PARSIMON_OPTIONS_H

#include <type_traits>
#include <iterator>

namespace anpa {

/**
 * Options that changes the behavior of a parser.
 *
 * See the documentation for each parser if any options are available.
 */
enum class options : uint64_t {
    none                  = 0,
    return_arg            = 1 << 0,
    dont_eat              = 1 << 1,
    include               = 1 << 2,
    fail_on_no_parse      = 1 << 3,
    negate                = 1 << 4,
    nested                = 1 << 5,
    no_negative           = 1 << 6,
    leading_plus          = 1 << 7,
    no_scientific         = 1 << 8,
    no_leading_zero       = 1 << 9,
    decimal_comma         = 1 << 10,
    optional              = 1 << 11,
    failure_only          = 1 << 12,
    no_trailing_separator = 1 << 13,
    ordered               = 1 << 14,
    replace               = 1 << 15,
};

/**
 * Combine two options
 */
constexpr inline options operator|(options lhs, options rhs) {
    using T = std::underlying_type_t<options>;
    return static_cast<options>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

/**
 * Check if `source` contains all options described by `flags`.
 */
constexpr inline bool has_options(options source, options flags) {
    using T = std::underlying_type_t<options>;
    return (static_cast<T>(source) & static_cast<T>(flags)) == static_cast<T>(flags);
}

}



#endif // PARSIMON_OPTIONS_H
