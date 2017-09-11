#include <iostream>
#include <vector>
#include <fstream>
#include <streambuf>
#include <memory>
#include "parser.h"

using namespace std;

int died = 0;

struct row {
    row(std::string_view firstName, std::string_view lastName) : firstName{firstName}, lastName{lastName} {}
//    ~row() {
//        ++died;
//        cout << died << endl;
//    }

    std::string firstName;
    std::string lastName;
};

int main()
{
    constexpr auto entry_parser = parse::string("Entry:") >> parse::until_token(':') >>=
            [=](auto firstName) {
        return parse::until_token(';') >>=
                [=](auto lastName) {
            return parse::succeed(parse::token('\n')) >>
                   parse::mreturn_emplace<row>(firstName, lastName);
        };
    };

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
