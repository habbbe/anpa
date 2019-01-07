#include <stack>
#include <iostream>
#include "test/catch.hpp"
#include "parser.h"

TEST_CASE("succeed") {
    REQUIRE(parse::succeed(parse::fail()).parse("").second);
    REQUIRE(parse::succeed(parse::success()).parse("").second);
}

struct with_error_handling {
    constexpr static bool error_handling = true;
};

TEST_CASE("change_error") {
    auto p = parse::change_error(parse::fail(), "new error");
    auto res = p.parse<with_error_handling>("");
    REQUIRE(res.second.has_error_handling);
    REQUIRE(res.second.error() == "new error");
}

TEST_CASE("is_empty") {
    auto p = parse::while_in("f");
    auto pNotEmpty = parse::not_empty(p);
    auto res = p.parse("abcde");
    auto resNotEmpty = pNotEmpty.parse("abcde");
    REQUIRE(res.second);
    REQUIRE(!resNotEmpty.second);
}

TEST_CASE("try_parser") {
    auto p = parse::try_parser(parse::sequence("abc") >> parse::sequence("df"));
    std::string str("abcde");
    auto res = p.parse(str);
    REQUIRE(!res.second);
    REQUIRE(res.first == str.begin());
}

TEST_CASE("no_consume") {
    auto p = parse::no_consume(parse::sequence("abcde"));
    std::string str("abcde");
    auto res = p.parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
    REQUIRE(res.first == str.begin());
}

TEST_CASE("constrain") {
    auto pred = [](auto &r) {
        return r == 1;
    };

    auto p = parse::constrain(parse::integer(), pred);

    auto res1 = p.parse("1");
    auto res2 = p.parse("2");

    REQUIRE(res1.second);
    REQUIRE(*res1.second == 1);

    REQUIRE(!res2.second);
}

TEST_CASE("get_parsed") {

    auto p1 = parse::get_parsed(parse::integer(), parse::sequence("abc"), parse::item('}'));
    auto p2 = parse::integer() + parse::sequence("abc") + parse::item('}');

    auto res1 = p1.parse("123abc}bc");
    auto res2 = p2.parse("123abc}bc");

    REQUIRE(res1.second);
    REQUIRE(*res1.second == "123abc}");

    REQUIRE(res2.second);
    REQUIRE(*res2.second == "123abc}");
}

TEST_CASE("first") {
    auto p1 = parse::first(parse::item('a'), parse::item('b'));
    auto p2 = parse::item('a') || parse::item('b');

    auto res1 = p1.parse("ab");
    auto res2 = p1.parse("ba");

    REQUIRE(res1.second);
    REQUIRE(*res1.second == 'a');

    REQUIRE(res2.second);
    REQUIRE(*res2.second == 'b');

    auto res3 = p2.parse("ab");
    auto res4 = p2.parse("ba");

    REQUIRE(res3.second);
    REQUIRE(*res3.second == 'a');

    REQUIRE(res4.second);
    REQUIRE(*res4.second == 'b');
}

TEST_CASE("modify_state") {
    auto p = parse::item('a') >>= [](auto &&r) {
        return parse::modify_state([=](auto &s) {
            s = 123;
            return r + 1;
        });
    };
    int state1 = 0;
    auto res1 = p.parse_with_state("abc", state1);

    REQUIRE(state1 == 123);
    REQUIRE(res1.second);
    REQUIRE(*res1.second == 'b');

    int state2 = 0;
    auto res2 = p.parse_with_state("bbc", state2);

    REQUIRE(state2 == 0);
    REQUIRE(!res2.second);
}

TEST_CASE("set_in_state") {
    auto p = parse::set_in_state(parse::item('a'), [](auto &s) -> auto& {return s;});
    int state1 = 0;

    auto res1 = p.parse_with_state("abc", state1);

    REQUIRE(state1 == 'a');
    REQUIRE(res1.second);
    REQUIRE(*res1.second == 'a');

    int state2 = 0;
    auto res2 = p.parse_with_state("bbc", state2);

    REQUIRE(state2 == 0);
    REQUIRE(!res2.second);
}

TEST_CASE("apply_to_state") {
    auto intParser = parse::item('#') >> parse::integer();
    auto p = parse::apply_to_state([](auto &s, auto i, auto j, auto k) {
        s = i + j + k;
        return 321;
    }, intParser, intParser, intParser);

    int state = 0;
    auto res = p.parse_with_state("#100#20#3", state);

    REQUIRE(state == 123);
    REQUIRE(res.second);
    REQUIRE(*res.second == 321);
}

TEST_CASE("emplace_to_state") {
    auto p = parse::emplace_to_state([](auto &s) -> auto& {
        return s;
    }, parse::sequence("abc") + parse::sequence("de"));

    std::stack<std::string> state;
    state.push("a");
    auto res = p.parse_with_state("abcdef", state);

    REQUIRE(state.size() == 2);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
}

TEST_CASE("emplace_to_state_direct") {
    auto p = parse::emplace_to_state_direct(parse::sequence("abc") + parse::sequence("de"));

    std::stack<std::string> state;
    state.push("a");
    auto res = p.parse_with_state("abcdef", state);

    REQUIRE(state.size() == 2);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
}

TEST_CASE("emplace_back_to_state") {
    auto p = parse::emplace_back_to_state([](auto &s) -> auto& {
        return s;
    }, parse::sequence("abc") + parse::sequence("de"));

    std::vector<std::string> state;
    state.push_back("a");
    auto res = p.parse_with_state("abcdef", state);

    REQUIRE(state.size() == 2);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
}

TEST_CASE("emplace_back_to_state_direct") {
    auto p = parse::emplace_back_to_state_direct(parse::sequence("abc") + parse::sequence("de"));

    std::vector<std::string> state;
    state.push_back("a");
    auto res = p.parse_with_state("abcdef", state);

    REQUIRE(state.size() == 2);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
}

TEST_CASE("many_to_vector") {
    std::string str("#100#20#3def");
    auto intParser = parse::item('#') >> parse::integer();
    auto p = parse::many_to_vector(intParser);
    auto res = p.parse(str);
    REQUIRE(res.second);
    REQUIRE(res.second->size() == 3);
    REQUIRE(res.second->at(0) == 100);
    REQUIRE(res.second->at(1) == 20);
    REQUIRE(res.second->at(2) == 3);
    REQUIRE(res.first == str.begin() + 9);
}

TEST_CASE("many_to_map") {
    std::string str("#1=a#2=b#3=c");
    auto pairParser = parse::lift_value<std::pair<int, char>>(parse::item('#') >> parse::integer(),
                                                              parse::item('=') >> parse::any_item());
    auto p = parse::many_to_map(pairParser);
    auto res = p.parse(str);
    REQUIRE(res.second);
    REQUIRE(res.second->size() == 3);
    REQUIRE(res.second->at(1) == 'a');
    REQUIRE(res.second->at(2) == 'b');
    REQUIRE(res.second->at(3) == 'c');
    REQUIRE(res.first == str.begin() + 12);
}

TEST_CASE("many_general") {
    std::string str("#1=a#2=b#3=c");
    auto pairParser = parse::lift_value<std::pair<int, char>>(parse::item('#') >> parse::integer(),
                                                              parse::item('=') >> parse::any_item());
    auto p = parse::many_general<std::unordered_map<int, char>>([](auto &m, auto &&r) {
        m.insert(r);
    }, pairParser);
    auto res = p.parse(str);
    REQUIRE(res.second);
    REQUIRE(res.second->size() == 3);
    REQUIRE(res.second->at(1) == 'a');
    REQUIRE(res.second->at(2) == 'b');
    REQUIRE(res.second->at(3) == 'c');
    REQUIRE(res.first == str.begin() + 12);
}

TEST_CASE("many_with_state") {
    auto intParser = parse::item('#') >> parse::integer();
    auto p = parse::many_state([](auto &s, auto i) {
        s.emplace_back(i);
    }, intParser);

    std::vector<int> state;
    auto res = p.parse_with_state("#100#20#3", state);

    REQUIRE(state.size() == 3);
    REQUIRE(res.second);
    REQUIRE(*res.second == "#100#20#3");
    REQUIRE(state[0] == 100);
    REQUIRE(state[1] == 20);
    REQUIRE(state[2] == 3);
}

TEST_CASE("lift_or") {
    auto atParser = parse::item('@') >> parse::integer();
    auto percentParser = parse::item('%') >> parse::any_item();
    auto hashParser = parse::item('#') >> parse::while_in("abc");

    struct f {
        auto operator()(int) {
            return 1;
        }

        auto operator()(char) {
            return 2;
        }

        auto operator()(std::string_view) {
            return 3;
        }
    };

    auto p = parse::lift_or(f(), atParser, percentParser, hashParser);

    auto res1 = p.parse("@123");
    REQUIRE(res1.second);
    REQUIRE(*res1.second == 1);

    auto res2 = p.parse("%d");
    REQUIRE(res2.second);
    REQUIRE(*res2.second == 2);

    auto res3 = p.parse("#aabbcc");
    REQUIRE(res3.second);
    REQUIRE(*res3.second == 3);
}

TEST_CASE("lift_or_state") {
    auto atParser = parse::item('@') >> parse::integer();
    auto percentParser = parse::item('%') >> parse::any_item();
    auto hashParser = parse::item('#') >> parse::while_in("abc");

    struct f {
        auto operator()(int &s, int) const {
            s = 11;
            return 1;
        }

        auto operator()(int &s, char) const {
            s = 22;
            return 2;
        }

        auto operator()(int &s, std::string_view) const {
            s = 33;
            return 3;
        }
    };

    auto p = parse::lift_or_state(f(), atParser, percentParser, hashParser);

    int state = 0;

    auto res1 = p.parse_with_state("@123", state);
    REQUIRE(res1.second);
    REQUIRE(*res1.second == 1);
    REQUIRE(state == 11);

    auto res2 = p.parse_with_state("%d", state);
    REQUIRE(res2.second);
    REQUIRE(*res2.second == 2);
    REQUIRE(state == 22);

    auto res3 = p.parse_with_state("#aabbcc", state);
    REQUIRE(res3.second);
    REQUIRE(*res3.second == 3);
    REQUIRE(state == 33);
}

TEST_CASE("lift_or_value") {
    auto atParser = parse::item('@') >> parse::rest(); // Conversion from string_view
    auto hashParser = parse::item('#') >> parse::lift_value<std::string>(parse::while_in("abc")); // Copy constructor

    auto p = parse::lift_or_value<std::string>(atParser, hashParser);

    auto res1 = p.parse("@123");
    REQUIRE(res1.second);
    REQUIRE(*res1.second == "123");

    auto res2 = p.parse("#aabbcc");
    REQUIRE(res2.second);
    REQUIRE(*res2.second == "aabbcc");
}

TEST_CASE("parse_result") {
    auto between = parse::between_items('{', '}');
    std::string_view str("{#100#20#3def}");
    auto intParser = parse::many_to_vector(parse::item('#') >> parse::integer());

    auto p = parse::parse_result(between, intParser);

    auto res = p.parse(str);

    REQUIRE(res.second);
    REQUIRE(res.second->size() == 3);
    REQUIRE(res.second->at(0) == 100);
    REQUIRE(res.second->at(1) == 20);
    REQUIRE(res.second->at(2) == 3);
    REQUIRE(res.first == str.end());
}

TEST_CASE("parse_result state") {
    auto between = parse::between_items('{', '}');
    std::string_view str("{#100#20#3def}");
    auto intParser = parse::many_to_vector(parse::item('#') >> parse::integer());

    auto p = parse::parse_result(between, intParser);

    int i = 0;

    auto res = p.parse_with_state(str, i);

    REQUIRE(res.second);
    REQUIRE(res.second->size() == 3);
    REQUIRE(res.second->at(0) == 100);
    REQUIRE(res.second->at(1) == 20);
    REQUIRE(res.second->at(2) == 3);
    REQUIRE(res.first == str.end());
}

TEST_CASE("until eat no include") {
    auto parseNumber = parse::parse_result(parse::between_items('{', '}'), parse::integer());
    auto p = parse::until(parseNumber);
    std::string_view str("abc{123}");

    auto res = p.parse(str);

    REQUIRE(res.second);
    REQUIRE(*res.second == "abc");
    REQUIRE(res.first == str.end());

    auto pFail = parse::until(parse::until_item('#'));

    auto resFail = pFail.parse(str);

    REQUIRE(!resFail.second);
    REQUIRE(resFail.first == str.begin());
}

TEST_CASE("until no eat no include") {
    auto parseNumber = parse::parse_result(parse::between_items('{', '}'), parse::integer());
    auto p = parse::until<false, false>(parseNumber);
    std::string_view str("abc{123}");

    auto res = p.parse(str);

    REQUIRE(res.second);
    REQUIRE(*res.second == "abc");
    REQUIRE(res.first == str.begin() + 3);
}

TEST_CASE("until eat include") {
    auto parseNumber = parse::parse_result(parse::between_items('{', '}'), parse::integer());
    auto p = parse::until<true, true>(parseNumber);
    std::string_view str("abc{123}");

    auto res = p.parse(str);

    REQUIRE(res.second);
    REQUIRE(*res.second == "abc{123}");
    REQUIRE(res.first == str.end());
}

TEST_CASE("until no eat include") {
    auto parseNumber = parse::parse_result(parse::between_items('{', '}'), parse::integer());
    auto p = parse::until<false, true>(parseNumber);
    std::string_view str("abc{123}");

    auto res = p.parse(str);

    REQUIRE(res.second);
    REQUIRE(*res.second == "abc{123}");
    REQUIRE(res.first == str.begin() + 3);
}

TEST_CASE("many_f with separator") {
    constexpr auto intParser = parse::integer();

    int result = 0;
    auto p = parse::many_f([&result](auto i) {
        result += i;
    }, intParser, parse::sequence("#%"));

    auto res = p.parse("100#%20#%3");

    REQUIRE(res.second);
    REQUIRE(*res.second == "100#%20#%3");
    REQUIRE(result == 123);
}

TEST_CASE("many_state with separator") {
    constexpr auto intParser = parse::integer();

    auto p = parse::many_state([](auto &s, auto i) {
        s += i;
    }, intParser, parse::sequence("#%"));

    int state = 0;
    auto res = p.parse_with_state("100#%20#%3", state);
    REQUIRE(res.second);
    REQUIRE(*res.second == "100#%20#%3");
    REQUIRE(state == 123);
}

TEST_CASE("many_to_vector with separator") {
    constexpr auto intParser = parse::integer();

    auto p = parse::many_to_vector(intParser, parse::sequence("#%"));

    int state = 0;
    auto res = p.parse_with_state("100#%20#%3", state);
    REQUIRE(res.second);
    REQUIRE(res.second->size() == 3);
    REQUIRE(res.second->at(0) == 100);
    REQUIRE(res.second->at(1) == 20);
    REQUIRE(res.second->at(2) == 3);
}
