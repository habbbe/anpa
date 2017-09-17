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
    action(command com) : com{com} {}
};

struct info {
    command com;
    info(command com) : com{com} {}
};

struct separator {};
struct space {};
struct syntax_error {
    syntax_error(std::string_view s) : description{s}{}
    std::string description;
};

using item = std::variant<
action,
info,
separator,
space,
syntax_error
>;

constexpr auto parse_name = parse::until_token('=');
constexpr auto parse_cmd = parse::not_empty(parse::rest());
constexpr auto parse_command = monad::lift_value_lazy_raw<command>(parse_name, parse_cmd);
constexpr auto parse_action = parse::string("Com:") >> monad::lift_value_lazy<action>(parse_command);
constexpr auto parse_info = parse::string("Info:") >> monad::lift_value_lazy<info>(parse_command);
constexpr auto parse_separator = parse::string("Separator") >> parse::empty() >> parse::mreturn(lazy::make_lazy(separator()));
constexpr auto parse_space = parse::string("Space") >> parse::empty() >> parse::mreturn(lazy::make_lazy(space()));
constexpr auto parse_error = monad::lift_value_lazy_raw<syntax_error>(parse::rest());
constexpr auto parse_item = parse::lift_or_value_from_lazy<item>(parse_action, parse_info, parse_separator, parse_space, parse_error);

int main()
{

    constexpr auto addToVector = [] (auto& args) {
        return [&] (auto& v) {
            v.emplace_back(std::move(args));
        };
    };

//    constexpr auto entry_parser = parse::string("Entry:") >> monad::lift(addToVector, parse::until_token(':'), parse::until_token(';'));
//    constexpr auto entry_parser = monad::lift(addToVector, parse_item);
    constexpr auto entry_parser = parse_item;

//    std::vector<row> r;
       std::vector<item> r;
//    std::ifstream t("test");
       std::ifstream t("hub");
    std::string line;
    while (std::getline(t, line)) {
        auto result = entry_parser(line);
        if (result.second) {
//            auto res = (*result.second)();
//            r.emplace_back(std::move((*result.second)()));
            r.emplace_back(std::move(*result.second));
//            (*result.second)(r);
        }
    }

    cout << "Size: " << r.size() << endl;
    cout << "Died: " << died << endl;

    return 0;
}
