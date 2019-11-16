#include <iostream>
#include <vector>
#include <fstream>
#include <streambuf>
#include <memory>
#include <variant>
#include <catch2/catch.hpp>
#include "parser.h"
#include "time_measure.h"

struct row {
    std::string_view firstName;
    std::string_view lastName;
    template <typename StringType1, typename StringType2>
    constexpr row(StringType1 firstName, StringType2 lastName) :
        firstName{firstName},
        lastName{lastName} {}
};

struct action {
    std::string_view name;
    std::string_view com;
    template <typename S1, typename S2>
    constexpr action(S1 name, S2 com) : name{name}, com{com} {}
};

struct info {
    std::string_view name;
    std::string_view com;
    template <typename S1, typename S2>
    constexpr info(S1 name, S2 com) : name{name}, com{com} {}
};

struct separator {};
struct space {};

struct syntax_error {
    std::string_view description;
    template <typename StringType>
    constexpr syntax_error(StringType s) : description(s) {}
};

using item = std::variant<
action,
info,
separator,
space,
syntax_error
>;

/**
 * Performance test for a simple syntax intended for a application launcher/information dashboard:
 *
 * Each row can be either of the following:
 *
 * Com:LABEL=COMMAND
 * Info:LABEL=COMMAND
 * Separator
 * Space
 */
double test()
{
    constexpr auto until_eol = parse::until_item<'\n', false>();
    constexpr auto parse_name = parse::until_item('=');
    constexpr auto parse_cmd = parse::not_empty(until_eol);
    constexpr auto parse_action = parse::sequence("Com:") >> parse::lift_value<action>(parse_name, parse_cmd);
    constexpr auto parse_info = parse::sequence("Info:") >> parse::lift_value<info>(parse_name, parse_cmd);
    constexpr auto parse_separator = parse::sequence("Separator") >> parse::mreturn_emplace<separator>();
    constexpr auto parse_space = parse::sequence("Space") >> parse::mreturn_emplace<space>();
    constexpr auto parse_comment = parse::item('#');
    constexpr auto parse_error = parse::lift_value<syntax_error>(until_eol);
    constexpr auto entry_parser = parse_comment || parse::lift_or_value<item>(parse_action, parse_info, parse_separator, parse_space, parse_error);
    constexpr auto parser = parse::many_to_vector<1000000>(entry_parser >> (parse::item<'\n'>() || parse::empty()));

    std::ifstream t("test_input/hub");
    std::string input((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());

    TICK;
    auto res = parser.parse(input);
    if (res.second) {
        std::cout << "Size: " << res.second.get_value().size() << std::endl;
    }
    TOCK("hub");

    return fp_ms.count();
}

TEST_CASE("performance") {
    test();
}
