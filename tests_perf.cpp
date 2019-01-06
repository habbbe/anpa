#include <iostream>
#include <vector>
#include <fstream>
#include <streambuf>
#include <memory>
#include <variant>
#include "test/catch.hpp"
#include "parser.h"
#include "time_measure.h"

struct row {
    template <typename StringType1, typename StringType2>
    constexpr row(StringType1 firstName, StringType2 lastName) : firstName{firstName}, lastName{lastName} {}

    std::string_view firstName;
    std::string_view lastName;
};

struct action {
    std::string_view name;
    std::string_view com;
    template <typename S1, typename S2>
    constexpr action(S1 &&name, S2 &&com) : name{std::forward<S1>(name)}, com{std::forward<S2>(com)} {}
};

struct info {
    std::string_view name;
    std::string_view com;
    template <typename S1, typename S2>
    constexpr info(S1 &&name, S2 &&com) : name{std::forward<S1>(name)}, com{std::forward<S2>(com)} {}
};

struct separator {};
struct space {};
struct syntax_error {
    template <typename StringType>
    constexpr syntax_error(StringType s) : description{s} {}
    std::string_view description;
};

using item = std::variant<
action,
info,
separator,
space,
syntax_error
>;

double test()
{
    constexpr auto add_to_state = [](auto &s, auto &&...args) {
        s.emplace_back(std::forward<decltype(args)>(args)...);
        return true;
    };

    constexpr auto parse_name = parse::until_item('=');
    constexpr auto parse_cmd = parse::not_empty(parse::rest());
    constexpr auto parse_action = parse::try_parser(parse::sequence("Com:") >> parse::lift_value<action>(parse_name, parse_cmd));
    constexpr auto parse_info = parse::try_parser(parse::sequence("Info:") >> parse::lift_value<info>(parse_name, parse_cmd));
    constexpr auto parse_separator = parse::sequence("Separator") >> parse::empty() >> parse::mreturn_forward<separator>();
    constexpr auto parse_space = parse::sequence("Space") >> parse::empty() >> parse::mreturn_forward<space>();
    constexpr auto parse_comment = parse::item('#');

    constexpr auto parse_error = parse::lift_value<syntax_error>(parse::rest());
    constexpr auto entry_parser = parse_comment || parse::lift_or_state(add_to_state, parse_action, parse_info, parse_separator, parse_space, parse_error);

    std::vector<item> r;
    r.reserve(1000000);
    std::ifstream t("hub");

//    constexpr auto add_to_state = [] (auto &s, auto&&... args) {
//        s.emplace_back(args...);
//        return true;
//    };
//    constexpr auto entry_parser = parse::string("Entry:") >> parse::apply_to_state(add_to_state, parse::until_token(':'), parse::until_token(';'));
//    std::vector<row> r;
//    std::ifstream t("test");
    std::vector<std::string> lines;
    lines.reserve(1000000);
    std::string line;
    while (std::getline(t, line)) {
        lines.push_back(line);
    }

    TICK;
    for (auto &l : lines) {
        entry_parser.parse_with_state(l.begin(), l.end(), r);
    }

    std::cout << "Size: " << r.size() << std::endl;
    TOCK("hub");

    return fp_ms.count();
}

TEST_CASE("performance") {
    test();
}
