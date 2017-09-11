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
    action(std::string name, std::string command, bool button) : com{name, command}, button{button}{}
};

struct separator {};
struct space {};
struct parse_error {
    std::string error;
    parse_error(std::string s) : error{s}{}
};

using item = std::variant<
action,
separator,
space,
parse_error
>;

auto parse_name = parse::until_token('=');
auto parse_cmd = parse::not_empty(parse::rest());
auto parse_command = parse::lift2<command>(parse_name, parse_cmd);
//auto parse_command = parse_name >>= [](auto name) {
//    return parse_cmd >>= [=] (auto cmd) {
//        return parse::mreturn_emplace<command>(name, cmd);
//    };
//};

//auto parse_action = parse::string("Com:") >> parse_command >>=

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
    constexpr auto entry_parser = (parse::string("Entry:") >>
                   parse::lift2<row>(parse::until_token(':'), parse::until_token(';')));

//    auto test = parser<decltype(entry_parser)>();

    std::vector<row> r;
    std::ifstream t("test");
    std::string line;
    while (std::getline(t, line)) {
        const auto &result = entry_parser(line);
        if (result.second) {
            r.push_back(*result.second);
//            r.push_back(*entry_parser(line).second);
        }
    }

    cout << "Size: " << r.size() << endl;
    cout << "Died: " << died << endl;

    return 0;
}
