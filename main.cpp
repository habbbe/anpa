#include <iostream>
#include <vector>
#include <fstream>
#include <streambuf>
#include <memory>
#include <variant>
#include "parser.h"

using namespace std;

int died = 0;

struct row {
    row(std::string_view firstName, std::string_view lastName) : firstName{firstName}, lastName{lastName} {}
    ~row() {
        ++died;
//        cout << died << endl;
    }

    std::string firstName;
    std::string lastName;
};

using command = std::pair<std::string, std::string>;

struct action {
    command com;
    bool button;
    action(command com, bool button) : com{com}, button{button}{}
};

struct separator {};
struct space {};
struct syntax_error {
    syntax_error(std::string_view s) : description{s}{}
    std::string description;
};

using item = std::variant<
action,
separator,
space,
syntax_error
>;

constexpr auto parse_name = parse::until_token('=');
constexpr auto parse_cmd = parse::not_empty(parse::rest());
constexpr auto parse_command = monad::lift_value<command>(parse_name, parse_cmd);
constexpr auto parse_action = parse::string("Com:") >> monad::lift_value<item>(monad::lift_value<action>(parse_command, parse::mreturn(true)));
constexpr auto parse_info = parse::string("Info:") >> monad::lift_value<item>(monad::lift_value<action>(parse_command, parse::mreturn(false)));
constexpr auto parse_separator = parse::string("Separator") >> parse::empty() >> monad::lift_value<item>(parse::mreturn_forward<separator>());
constexpr auto parse_space = parse::string("Space") >> parse::empty() >> monad::lift_value<item>(parse::mreturn_forward<space>());
constexpr auto parse_error = monad::lift_value<item>(monad::lift_value<syntax_error>(parse::rest()));
constexpr auto parse_item = parse_action || parse_info || parse_separator || parse_space || parse_error;

int main()
{
    constexpr auto addToVector = [] (auto&&... args) {
        return [=] (auto&& v) {
            v.emplace_back(args...);
        };
    };

    constexpr auto entry_parser = parse::string("Entry:") >> monad::lift(addToVector, parse::until_token(':'), parse::until_token(';'));
//    constexpr auto entry_parser = monad::lift(addToVector, parse_item);
//    constexpr auto entry_parser = parse_item;

    std::vector<row> r;
//       std::vector<item> r;
    std::ifstream t("test");
//       std::ifstream t("hub");
    std::string line;
    while (std::getline(t, line)) {
        auto result = entry_parser(line);
        if (result.second) {
            auto res = *result.second;
//            res(r);
//            r.push_back(res);
            res(r);
//            res()(r);
        }
    }

//    for (auto &i : r) {
//        std::visit([](auto &&v) {
//            using T = std::decay_t<decltype(v)>;
//            if constexpr (std::is_same_v<T, action>) {
//                cout << (v.button ? "button" : "info") << endl;
//            } else if constexpr (std::is_same_v<T, separator>) {
//                cout << "separator" << endl;
//            } else if constexpr (std::is_same_v<T, space>) {
//                cout << "space" << endl;
//            } else if constexpr (std::is_same_v<T, syntax_error>) {
//                cout << "error" << endl;
//            }
//        }, i.v);
//    }

    cout << "Size: " << r.size() << endl;
    cout << "Died: " << died << endl;

    return 0;
}
