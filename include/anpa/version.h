#ifndef PARSIMON_VERSION_H
#define PARSIMON_VERSION_H

#include "anpa/parsers.h"

namespace anpa::version {

/// Current version of anpa as a std::string_view
// MAJOR.MINOR.PATCH-PRE_RELEASE components are parsed from this during compile time
constexpr std::string_view current = "0.5.0";

struct version_components {
    unsigned int major;
    unsigned int minor;
    unsigned int patch;
    std::string_view pre_release;
};

constexpr auto version_parser = []{
    auto num = integer<unsigned int, options::no_leading_zero>();
    auto next_num = item<'.'>() >> num;
    return lift_value<version_components>(
            num,
            next_num,
            next_num,
            item<'-'>() >> not_empty(rest()) || empty());
}();


constexpr auto components = [] {
    constexpr auto version = version_parser.parse(current).second;
    static_assert (version.has_value(), "Version string is malformed");
    return *version;
}();

}



#endif // PARSIMON_VERSION_H
