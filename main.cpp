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

auto parse_name = parse::until_token('=');
auto parse_cmd = parse::not_empty(parse::rest());
auto parse_command = monad::lift_value<command>(parse_name, parse_cmd);
auto parse_action = parse::string("Com:") >> monad::lift_value<item>(monad::lift_value<action>(parse_command, parse::mreturn(true)));
auto parse_info = parse::string("Info:") >> monad::lift_value<item>(monad::lift_value<action>(parse_command, parse::mreturn(false)));
auto parse_separator = parse::string("Separator") >> parse::empty() >> monad::lift_value<item>(parse::mreturn(separator()));
auto parse_space = parse::string("Space") >> parse::empty() >> monad::lift_value<item>(parse::mreturn(space()));
auto parse_error = monad::lift_value<item>(monad::lift_value<syntax_error>(parse::rest()));
auto parse_item = parse_action || parse_info || parse_separator || parse_space || parse_error;

int main()
{
//    constexpr auto entry_parser = parse::string("Entry:") >> parse::until_token(':') >>=
//            [=](auto firstName) {
//        return parse::until_token(';') >>=
//                [=](auto lastName) {
//            return parse::succeed(parse::token('\n')) >>
//                   parse::mreturn_emplace<row>(firstName, lastName);
//        };
//    };

   constexpr auto entry_parser = parse::string("Entry:") >>
                   monad::lift_value_lazy<row>(parse::until_token(':'), parse::until_token(';'));

    std::vector<row> r;


    std::ifstream t("test");
    std::string line;
    while (std::getline(t, line)) {
        auto result = entry_parser(line);
        if (result.second) {
            auto res = *result.second;
            r.push_back(res());
        }
    }

//    for (auto &row : r) {
//        cout << row.firstName << " " << row.lastName << endl;
//    }

    cout << "Size: " << r.size() << endl;
    cout << "Died: " << died << endl;

    return 0;
}
