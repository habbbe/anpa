#include <iostream>
#include <vector>
#include <fstream>
#include <streambuf>
#include <memory>
#include <variant>
#include "parser.h"

//using namespace std;

struct row {
    template <typename StringType1, typename StringType2>
    row(StringType1 firstName, StringType2 lastName) : firstName{firstName}, lastName{lastName} {}

    std::string firstName;
    std::string lastName;
};

struct action {
    std::string name;
    std::string com;
    template <typename S1, typename S2>
    action(S1 &&name, S2 &&com) : name{std::forward<S1>(name)}, com{std::forward<S2>(com)} {}
};

struct info {
    std::string name;
    std::string com;
    template <typename S1, typename S2>
    info(S1 &&name, S2 &&com) : name{std::forward<S1>(name)}, com{std::forward<S2>(com)} {}
};

struct separator {};
struct space {};
struct syntax_error {
    template <typename StringType>
    syntax_error(StringType s) : description{s} {}
    std::string description;
};

using item = std::variant<
action,
info,
separator,
space,
syntax_error
>;

int main()
{
//    auto test = get_array_parser();

    constexpr auto add_to_state = [] (auto &s, auto&&... args) {
        s.emplace_back(std::move(args)...);
        return true;
    };

    constexpr auto parse_name = parse::until_token('=');
    constexpr auto parse_cmd = parse::not_empty(parse::rest());
    constexpr auto parse_action = parse::try_parser(parse::string("Com:") >> monad::lift_value<action>(parse_name, parse_cmd));
    constexpr auto parse_info = parse::try_parser(parse::string("Info:") >> monad::lift_value<info>(parse_name, parse_cmd));
    constexpr auto parse_separator = parse::string("Separator") >> parse::empty() >> parse::mreturn_forward<separator>();
    constexpr auto parse_space = parse::string("Space") >> parse::empty() >> parse::mreturn_forward<space>();
    constexpr auto parse_error = monad::lift_value<syntax_error>(parse::rest());
    constexpr auto entry_parser = parse::lift_or_state(add_to_state, parse_action, parse_info, parse_separator, parse_space, parse_error);

    parse::parser test = parse::until_token('=');

    std::vector<item> r;
    std::ifstream t("hub");

//    constexpr auto add_to_state = [] (auto &s, auto&&... args) {
//        s.emplace_back(args...);
//        return true;
//    };
//    constexpr auto entry_parser = parse::string("Entry:") >> parse::apply_to_state(add_to_state, parse::until_token(':'), parse::until_token(';'));
//    std::vector<row> r;
//    std::ifstream t("test");

    std::string line;
    while (std::getline(t, line)) {
        entry_parser.parse_with_state(std::string_view(line), r);
//        auto result = entry_parser.parse(std::string_view(line));
//        if (result.second) {
//            r.emplace_back(std::move(*result.second));
//        }
    }

    std::cout << "Size: " << r.size() << std::endl;

    return 0;
}
