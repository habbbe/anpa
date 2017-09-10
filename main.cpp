#include <iostream>
#include <vector>
#include <fstream>
#include <streambuf>
#include "parser.h"

using namespace std;

struct row {
    row(std::string_view firstName, std::string_view lastName) : firstName{firstName}, lastName{lastName} {}

    std::string firstName;
    std::string lastName;
};

int main()
{
    auto entry_parser = parse::string("Entry:") >> parse::until_token(':') >>=
            [=](auto firstName) {
        return parse::until_token(';') >>=
                [=](auto lastName) {
                    return parse::succeed(parse::token('\n')) >>
                           parse::mreturn(std::make_pair(firstName, lastName));
        };
    };

    std::vector<row> r;
    std::ifstream t("test");
    std::string line;
    while (std::getline(t, line)) {
        auto result = entry_parser(line);
        if (result.second)
            r.emplace_back(result.second->first, result.second->second);
    }
    cout << "Size: " << r.size() << endl;

    return 0;
}
