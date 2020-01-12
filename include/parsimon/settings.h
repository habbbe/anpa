#ifndef PARSIMON_SETTINGS_H
#define PARSIMON_SETTINGS_H

#include <type_traits>
#include <iterator>
#include <array>
#include "parsimon/range.h"

namespace parsimon {

/// Conversion function that returns a `range`
constexpr auto range_convert = [](auto begin, auto end) {
    return range(begin, end);
};

/**
 * Parser settings.
 *
 * @tparam ErrorMessages set if error messages are desired.
 * @tparam Convert the functor to be used to create results for parsed ranges.
 *         It should have the following signature:
 *           `ResultType(auto begin_iterator, auto end_iterator)`
 */
template <bool ErrorMessages = false, auto& Convert = range_convert>
struct parser_settings {
    constexpr static bool error_messages = ErrorMessages;
    constexpr static auto conversion_function = Convert;
};

/**
 * The default parser settings. No error messages, and range results are
 * returned as `range<Iterator>`
 */

using default_parser_settings = parser_settings<>;

}

#endif // PARSIMON_SETTINGS_H
