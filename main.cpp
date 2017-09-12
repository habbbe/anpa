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
auto parse_command = parse::lift2<command>(parse_name, parse_cmd);
auto parse_action = parse::string("Com:") >> parse::lift<item>(parse::lift2<action>(parse_command, parse::mreturn(true)));
auto parse_info = parse::string("Info:") >> parse::lift<item>(parse::lift2<action>(parse_command, parse::mreturn(false)));
auto parse_separator = parse::string("Separator") >> parse::empty() >> parse::lift<item>(parse::mreturn(separator()));
auto parse_space = parse::string("Space") >> parse::empty() >> parse::lift<item>(parse::mreturn(space()));
auto parse_error = parse::lift<item>(parse::lift<syntax_error>(parse::rest()));
auto parse_item = parse_action || parse_info || parse_separator || parse_space || parse_error;

int main()
{
    std::vector<item> r;
    std::ifstream t("/home/habbbe/.hub");
    std::string line;
    while (std::getline(t, line)) {
        const auto &result = parse_item(line);
        if (result.second) {
            auto &i = *result.second;
            r.push_back(i);
        }
    }
    for (auto &i : r) {
        std::visit([](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, action>) {
                            if (arg.button)
                                std::cout << "Action with name: " << arg.com.first << " command: " << arg.com.second << '\n';
                            else
                                std::cout << "Info with name: " << arg.com.first << " command: " << arg.com.second << '\n';
                    } else if constexpr (std::is_same_v<T, space>)
                        std::cout << "Space" << '\n';
                    else if constexpr (std::is_same_v<T, separator>)
                        std::cout << "Separator" << '\n';
                    else if constexpr (std::is_same_v<T, syntax_error>)
                        std::cout << "Syntax error: " << arg.description << '\n';
                }, i);
    }

//    std::vector<row> r;
//    std::ifstream t("test");
//    std::string line;
//    while (std::getline(t, line)) {
//        const auto &result = entry_parser(line);
//        if (result.second) {
//            r.push_back(*result.second);
//        }
//    }

    cout << "Size: " << r.size() << endl;
    cout << "Died: " << died << endl;

    return 0;
}
