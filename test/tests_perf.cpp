#include <iostream>
#include <vector>
#include <fstream>
#include <streambuf>
#include <memory>
#include <variant>
#include <catch2/catch.hpp>
#include "parsimon/parsimon.h"
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
 * # COMMENT
 *
 *
 * This parser uses an external state and is invoked on each line of input.
 *
 */
void test()
{
    using namespace parsimon;

    constexpr auto add_to_state = [](auto& s, auto&& arg) {
        s.emplace_back(std::forward<decltype(arg)>(arg));
    };

    constexpr auto parse_name = until_item('=');
    constexpr auto parse_cmd = not_empty(rest());
    constexpr auto parse_action = seq("Com:") >> lift_value<action>(parse_name, parse_cmd);
    constexpr auto parse_info = seq("Info:") >> lift_value<info>(parse_name, parse_cmd);
    constexpr auto parse_separator = seq("Separator") >> mreturn_emplace<separator>();
    constexpr auto parse_space = seq("Space") >> mreturn_emplace<space>();
    constexpr auto parse_error = lift_value<syntax_error>(rest());
    constexpr auto ignore = empty() || (item('#') >> rest());
    constexpr auto entry_parser = ignore || lift_or_state(add_to_state, parse_action, parse_info, parse_separator, parse_space, parse_error);

    std::ifstream t("hub");

    std::vector<entry> state;
    state.reserve(1000000);

    std::vector<std::string> lines;
    lines.reserve(1000000);
    std::string line;
    while (std::getline(t, line)) {
        lines.push_back(line);
    }

    TICK;

    for (auto& l : lines) {
        entry_parser.parse_with_state(l, state);
    }

    std::cout << "Size: " << state.size() << std::endl;
    TOCK("hub");
}

TEST_CASE("performance") {
    test();
}
