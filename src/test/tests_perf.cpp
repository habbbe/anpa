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

using entry = std::variant<
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
    using namespace parsimon;
    constexpr auto until_eol = until_item<'\n', false>();
    constexpr auto parse_name = until_item('=');
    constexpr auto parse_cmd = not_empty(until_eol);
    constexpr auto parse_action = sequence("Com:") >> lift_value<action>(parse_name, parse_cmd);
    constexpr auto parse_info = sequence("Info:") >> lift_value<info>(parse_name, parse_cmd);
    constexpr auto parse_separator = sequence("Separator") >> mreturn_emplace<separator>();
    constexpr auto parse_space = sequence("Space") >> mreturn_emplace<space>();
    constexpr auto parse_comment = item('#');
    constexpr auto parse_error = lift_value<syntax_error>(until_eol);
    constexpr auto entry_parser = parse_comment || lift_or_value<entry>(parse_action, parse_info, parse_separator, parse_space, parse_error);
    constexpr auto parser = many_to_vector<1000000>(entry_parser >> (item<'\n'>() || empty()));

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
