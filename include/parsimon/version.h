#ifndef PARSIMON_VERSION_H
#define PARSIMON_VERSION_H

#include "parsimon/parsers.h"

namespace parsimon::version {

/// Current version of parsimon as a std::string_view
// MAJOR.MINOR.PATCH-PRE_RELEASE components are parsed from this during compile time
constexpr std::string_view current = "0.1.0";

struct version_components {
    unsigned int major;
    unsigned int minor;
    unsigned int patch;
    std::string_view pre_release;

    constexpr version_components(unsigned int major,
                                 unsigned int minor,
                                 unsigned int patch,
                                 std::string_view pre_release) :
        major{major}, minor{minor}, patch{patch}, pre_release{pre_release} {}
};

constexpr auto version_parser = lift_value<version_components>(
            integer<unsigned int>(),
            item<'.'>() >> integer<unsigned int>(),
            item<'.'>() >> integer<unsigned int>(),
            item<'-'>() >> not_empty(rest()) || empty() >= std::string_view{});

namespace components {

constexpr auto internal_components = []() {
    constexpr auto version = version_parser.parse(current).second;
    static_assert (version.has_value(), "Version string is malformed");
    return *version;
}();

constexpr auto major = internal_components.major;
constexpr auto minor = internal_components.minor;
constexpr auto patch = internal_components.patch;
constexpr auto pre_release = internal_components.pre_release;

}

}



#endif // PARSIMON_VERSION_H
