#include <stack>
#include <iostream>
#include <functional>
#include <string>
#include <catch2/catch.hpp>
#include "parsimon/parsimon.h"

TEST_CASE("succeed") {
    using namespace parsimon;
    static_assert(succeed(fail()).parse("").second);
    static_assert(succeed(success()).parse("").second);
}

struct with_error_handling {
    constexpr static bool error_handling = true;
    constexpr static auto conversion_function = parsimon::string_view_convert;
};

TEST_CASE("change_error") {
    using namespace parsimon;
    constexpr auto p = change_error(fail(), "new error");
    constexpr auto res = p.parse<with_error_handling>(std::string_view(""));
    static_assert(res.second.has_error_handling);
    static_assert(res.second.error() == std::string_view("new error"));
}

TEST_CASE("not_empty") {
    using namespace parsimon;
    constexpr auto p = while_in("f");
    constexpr auto pNotEmpty = not_empty(p);
    constexpr auto res = p.parse("abcde");
    constexpr auto resNotEmpty = pNotEmpty.parse("abcde");
    static_assert(res.second);
    static_assert(!resNotEmpty.second);
}

TEST_CASE("try_parser") {
    using namespace parsimon;
    constexpr auto p = try_parser(sequence("abc") >> sequence("df"));
    constexpr std::string_view str("abcde");
    constexpr auto res = p.parse(str);
    static_assert(!res.second);
    static_assert(res.first.position == str.begin());
}

TEST_CASE("no_consume") {
    using namespace parsimon;
    constexpr auto p = no_consume(sequence("abcde"));
    constexpr std::string_view str("abcde");
    constexpr auto res = p.parse(str);
    static_assert(res.second);
    static_assert(*res.second == "abcde");
    static_assert(res.first.position == str.begin());
}

TEST_CASE("constrain") {
    using namespace parsimon;
    auto pred = [](auto& r) {
        return r == 1;
    };

    constexpr auto p = constrain(integer(), pred);

    constexpr auto res1 = p.parse("1");
    constexpr auto res2 = p.parse("2");

    static_assert(res1.second);
    static_assert(*res1.second == 1);

    static_assert(!res2.second);
}

TEST_CASE("get_parsed") {

    using namespace parsimon;
    constexpr auto p1 = get_parsed(integer(), sequence("abc"), item('}'));
    constexpr auto p2 = integer() + sequence("abc") + item('}');

    constexpr std::string_view str("123abc}bc");

    constexpr auto res1 = p1.parse(str);
    constexpr auto res2 = p2.parse(str);

    static_assert(res1.second);
    static_assert(*res1.second == "123abc}");
    static_assert(res1.first.position == str.end() - 2);

    static_assert(res2.second);
    static_assert(*res2.second == "123abc}");
    static_assert(res2.first.position == str.end() - 2);
}

TEST_CASE("first") {
    using namespace parsimon;
    constexpr auto p1 = first(item('a'), item('b'));
    constexpr auto p2 = item('a') || item('b');

    constexpr std::string_view str1("ab");
    constexpr std::string_view str2("ba");

    constexpr auto res1 = p1.parse(str1);
    constexpr auto res2 = p1.parse(str2);

    static_assert(res1.second);
    static_assert(*res1.second == 'a');
    static_assert(res1.first.position == str1.begin() + 1);

    static_assert(res2.second);
    static_assert(*res2.second == 'b');
    static_assert(res2.first.position == str2.begin() + 1);

    constexpr auto res3 = p2.parse(str1);
    constexpr auto res4 = p2.parse(str2);

    static_assert(res3.second);
    static_assert(*res3.second == 'a');
    static_assert(res3.first.position == str1.begin() + 1);

    static_assert(res4.second);
    static_assert(*res4.second == 'b');
    static_assert(res4.first.position == str2.begin() + 1);
}

TEST_CASE("with_state constexpr") {
    using namespace parsimon;
    struct state {
        char x = 'a';
    };

    constexpr auto char_progression = with_state([](auto &s) {
        char c = s.x;
        s.x++;
        return item(c);
    });

    constexpr auto p = many(char_progression, item<' '>());

    constexpr auto res1 = p.parse_with_state("a b c d e f", state());

    static_assert(res1.second);
    static_assert (*res1.second == "a b c d e f");

    constexpr auto res2 = p.parse_with_state("a b c c d e", state());

    static_assert(res2.second);
    static_assert (*res2.second == "a b c ");
}

TEST_CASE("with_state") {
    using namespace parsimon;
    struct state {
        int n = 0;
        int step = 1;
        constexpr state(int start, int step) : n{start}, step{step} {}
    };

    constexpr auto int_progression = with_state([](auto &s) {

        constexpr auto parse_int = [](int i) {
            auto str = std::to_string(i);
            return sequence(str.begin(), str.end());
        };

        int i = s.n;
        s.n += s.step;
        return parse_int(i);
    });

    constexpr auto p = many(int_progression, item<' '>());

    auto res1 = p.parse_with_state("1 2 3 4 5 6 7", state(1, 1));

    REQUIRE(res1.second);
    REQUIRE(*res1.second == "1 2 3 4 5 6 7");

    auto res2 = p.parse_with_state("2 5 8 11 14 17", state(2, 3));

    REQUIRE(res2.second);
    REQUIRE(*res2.second == "2 5 8 11 14 17");
}


TEST_CASE("modify_state") {
    using namespace parsimon;
    constexpr auto p = item('a') >>= [](auto&& r) {
        return modify_state([=](auto& s) {
            s = 123;
            return r + 1;
        });
    };
    constexpr auto res1 = p.parse_with_state("abc", 0);

    static_assert(res1.first.user_state == 123);
    static_assert(res1.second);
    static_assert(*res1.second == 'b');

    constexpr auto res2 = p.parse_with_state("bbc", 0);

    static_assert(res2.first.user_state == 0);
    static_assert(!res2.second);
}

TEST_CASE("set_in_state") {
    using namespace parsimon;
    struct state {
        char c = 'b';
    };

    constexpr auto p = set_in_state(item('a'), [](auto& s) -> auto& {return s.c;});

    constexpr auto res1 = p.parse_with_state("abc", state());

    static_assert(res1.first.user_state.c == 'a');
    static_assert(res1.second);
    static_assert(*res1.second == 'a');

    constexpr auto res2 = p.parse_with_state("bbc", state());

    static_assert(res2.first.user_state.c == 'b');
    static_assert(!res2.second);
}

TEST_CASE("apply_to_state") {
    using namespace parsimon;
    constexpr auto intParser = item('#') >> integer();
    constexpr auto p = apply_to_state([](auto& s, auto i, auto j, auto k) {
        s = i + j + k;
        return 321;
    }, intParser, intParser, intParser);

    constexpr auto res = p.parse_with_state("#100#20#3", 0);

    static_assert(res.first.user_state == 123);
    static_assert(res.second);
    static_assert(*res.second == 321);
}

TEST_CASE("emplace_to_state") {
    using namespace parsimon;
    auto p = emplace_to_state([](auto& s) -> auto& {
        return s;
    }, sequence("abc") + sequence("de"));

    std::stack<std::string> state;
    state.push("a");
    auto res = p.parse_with_state("abcdef", state);

    REQUIRE(state.size() == 2);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
}

TEST_CASE("emplace_to_state_direct") {
    using namespace parsimon;
    auto p = emplace_to_state_direct(sequence("abc") + sequence("de"));

    std::stack<std::string> state;
    state.push("a");
    auto res = p.parse_with_state("abcdef", state);

    REQUIRE(state.size() == 2);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
}

TEST_CASE("emplace_back_to_state") {
    using namespace parsimon;
    auto p = emplace_to_state<true>([](auto& s) -> auto& {
        return s;
    }, sequence("abc") + sequence("de"));

    std::vector<std::string> state;
    state.push_back("a");
    auto res = p.parse_with_state("abcdef", state);

    REQUIRE(state.size() == 2);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
}

TEST_CASE("emplace_back_to_state_direct") {
    using namespace parsimon;
    auto p = emplace_to_state_direct<true>(sequence("abc") + sequence("de"));

    std::vector<std::string> state;
    state.push_back("a");
    auto res = p.parse_with_state("abcdef", state);

    REQUIRE(state.size() == 2);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
}

TEST_CASE("many_to_vector") {
    std::string str("#100#20#3def");
    auto intParser = parsimon::item('#') >> parsimon::integer();
    auto p = parsimon::many_to_vector(intParser);
    auto res = p.parse(str);
    REQUIRE(res.second);
    REQUIRE(res.second->size() == 3);
    REQUIRE(res.second->at(0) == 100);
    REQUIRE(res.second->at(1) == 20);
    REQUIRE(res.second->at(2) == 3);
    REQUIRE(res.first.position == str.begin() + 9);
}

TEST_CASE("many_to_array") {
    using namespace parsimon;
    constexpr std::string_view str("#100#20#3def");
    constexpr auto intParser = item('#') >> integer();
    constexpr auto p = many_to_array<100>(intParser);
    constexpr auto res = p.parse(str);
    static_assert(res.second);
    static_assert(res.second->second == 3);
    static_assert(res.second->first[0] == 100);
    static_assert(res.second->first[1] == 20);
    static_assert(res.second->first[2] == 3);
    static_assert(res.first.position == str.begin() + 9);
}

TEST_CASE("many_to_array with separator") {
    using namespace parsimon;
    constexpr auto intParser = integer();

    constexpr auto p = many_to_array<10>(intParser, sequence("#%"));

    constexpr auto res = p.parse_with_state("100#%20#%3", 0);
    static_assert(res.second);
    static_assert(res.second->second == 3);
    static_assert(res.second->first[0] == 100);
    static_assert(res.second->first[1] == 20);
    static_assert(res.second->first[2] == 3);
}


TEST_CASE("many_to_map") {
    std::string str("#1=a#2=b#3=c");
    auto pairParser = parsimon::lift_value<std::pair<int, char>>(parsimon::item('#') >> parsimon::integer(),
                                                                 parsimon::item('=') >> parsimon::any_item());
    auto p = parsimon::many_to_map(pairParser);
    auto res = p.parse(str);
    REQUIRE(res.second);
    REQUIRE(res.second->size() == 3);
    REQUIRE(res.second->at(1) == 'a');
    REQUIRE(res.second->at(2) == 'b');
    REQUIRE(res.second->at(3) == 'c');
    REQUIRE(res.first.position == str.begin() + 12);
}

TEST_CASE("many_general") {
    using namespace parsimon;
    struct val {
        int i = 0;
        char is[100] = {};
    };
    constexpr std::string_view str("#1=a#4=b#7=c");
    constexpr auto pairParser = lift_value<std::pair<int, char>>(item('#') >> integer(),
                                                              item('=') >> any_item());
    constexpr auto p = many_general<val>(pairParser, [](auto& s, auto&& r) {
        s.is[r.first] = r.second;
    });

    constexpr auto res = p.parse(str);
    static_assert(res.second);
    static_assert(res.second->is[1] == 'a');
    static_assert(res.second->is[4] == 'b');
    static_assert(res.second->is[7] == 'c');
    static_assert(res.first.position == str.begin() + 12);
}

TEST_CASE("many_state") {
    using namespace parsimon;
    struct state {
        int i = 0;
        int is[100] = {};
    };

    constexpr auto intParser = item('#') >> integer();
    constexpr auto p = many_state(intParser, [](auto& s, auto i) {
        s.is[s.i++] = i;
    });
    constexpr auto res = p.parse_with_state("#100#20#3", state());
    static_assert(res.second);
    static_assert(*res.second == "#100#20#3");

    static_assert(res.first.user_state.i == 3);
    static_assert(res.first.user_state.is[0] == 100);
    static_assert(res.first.user_state.is[1] == 20);
    static_assert(res.first.user_state.is[2] == 3);
}

TEST_CASE("fold") {
    using namespace parsimon;
    constexpr std::string_view str("#100#20#3");
    constexpr auto intParser = item<'#'>() >> integer();
    constexpr auto p = fold(intParser, 0, [](auto a, auto b) {return a + b;});
    constexpr auto res = p.parse(str);
    static_assert(res.second);
    static_assert(*res.second == 123);
}

TEST_CASE("lift_or") {
    using namespace parsimon;
    constexpr auto atParser = item('@') >> integer();
    constexpr auto percentParser = item('%') >> any_item();
    constexpr auto hashParser = item('#') >> while_in("abc");

    struct f {
        constexpr auto operator()(int) {
            return 1;
        }

        constexpr auto operator()(char) {
            return 2;
        }

        constexpr auto operator()(std::string_view) {
            return 3;
        }
    };

    constexpr auto p = lift_or(f(), atParser, percentParser, hashParser);

    constexpr auto res1 = p.parse("@123");
    static_assert(res1.second);
    static_assert(*res1.second == 1);

    constexpr auto res2 = p.parse("%d");
    static_assert(res2.second);
    static_assert(*res2.second == 2);

    constexpr auto res3 = p.parse("#aabbcc");
    static_assert(res3.second);
    static_assert(*res3.second == 3);
}

TEST_CASE("lift_or_state") {
    using namespace parsimon;
    constexpr auto atParser = item('@') >> integer();
    constexpr auto percentParser = item('%') >> any_item();
    constexpr auto hashParser = item('#') >> while_in("abc");

    struct f {
        constexpr auto operator()(int& s, int) const {
            s = 11;
            return 1;
        }

        constexpr auto operator()(int& s, char) const {
            s = 22;
            return 2;
        }

        constexpr auto operator()(int& s, std::string_view) const {
            s = 33;
            return 3;
        }
    };

    constexpr auto p = lift_or_state(f(), atParser, percentParser, hashParser);


    constexpr auto res1 = p.parse_with_state("@123", 0);
    static_assert(res1.second);
    static_assert(*res1.second == 1);
    static_assert(res1.first.user_state == 11);

    constexpr auto res2 = p.parse_with_state("%d", 0);
    static_assert(res2.second);
    static_assert(*res2.second == 2);
    static_assert(res2.first.user_state == 22);

    constexpr auto res3 = p.parse_with_state("#aabbcc", 0);
    static_assert(res3.second);
    static_assert(*res3.second == 3);
    static_assert(res3.first.user_state == 33);
}

TEST_CASE("lift_or_value") {
    using namespace parsimon;
    struct t {
        size_t i;
        constexpr t(std::string_view s): i{s.length()} {}
        constexpr t(size_t i): i{i} {}
    };

    constexpr auto atParser = item('@') >> rest();
    constexpr auto hashParser = item('#') >> integer();

    constexpr auto p = lift_or_value<t>(atParser, hashParser);

    constexpr auto res1 = p.parse("@123");
    static_assert(res1.second);
    static_assert(res1.second->i == 3);

    constexpr auto res2 = p.parse("#1234");
    static_assert(res2.second);
    static_assert(res2.second->i == 1234);
}

TEST_CASE("parse_result") {
    using namespace parsimon;
    constexpr auto between = between_items('{', '}');
    constexpr std::string_view str("{#100#20#3def}");
    constexpr auto intParser = many_to_array<10>(item('#') >> integer());

    constexpr auto p = parse_result(between, intParser);

    constexpr auto res = p.parse(str);

    static_assert(res.second);
    static_assert(res.second->second == 3);
    static_assert(res.second->first[0] == 100);
    static_assert(res.second->first[1] == 20);
    static_assert(res.second->first[2] == 3);
    static_assert(res.first.position == str.end());
}

TEST_CASE("parse_result state") {
    using namespace parsimon;
    constexpr auto between = between_items('{', '}');
    constexpr std::string_view str("{#100#20#3def}");
    constexpr auto intParser = many_to_array<10>(item('#') >> integer());

    constexpr auto p = parse_result(between, intParser);

    constexpr auto res = p.parse_with_state(str, 0);

    static_assert(res.second);
    static_assert(res.second->second == 3);
    static_assert(res.second->first[0] == 100);
    static_assert(res.second->first[1] == 20);
    static_assert(res.second->first[2] == 3);
    static_assert(res.first.position == str.end());
}

TEST_CASE("until eat no include") {
    using namespace parsimon;
    constexpr auto parseNumber = parse_result(between_items('{', '}'), integer());
    constexpr auto p = until(parseNumber);
    constexpr std::string_view str("abc{123}");

    constexpr auto res = p.parse(str);

    static_assert(res.second);
    static_assert(*res.second == "abc");
    static_assert(res.first.position == str.end());

    constexpr auto pFail = until(until_item('#'));

    constexpr auto resFail = pFail.parse(str);

    static_assert(!resFail.second);
    static_assert(resFail.first.position == str.begin());
}

TEST_CASE("until no eat no include") {
    using namespace parsimon;
    constexpr auto parseNumber = parse_result(between_items('{', '}'), integer());
    constexpr auto p = until<false, false>(parseNumber);
    constexpr std::string_view str("abc{123}");

    constexpr auto res = p.parse(str);

    static_assert(res.second);
    static_assert(*res.second == "abc");
    static_assert(res.first.position == str.begin() + 3);
}

TEST_CASE("until eat include") {
    using namespace parsimon;
    constexpr auto parseNumber = parse_result(between_items('{', '}'), integer());
    constexpr auto p = until<true, true>(parseNumber);
    constexpr std::string_view str("abc{123}");
    constexpr auto res = p.parse(str);

    static_assert(res.second);
    static_assert(*res.second == "abc{123}");
    static_assert(res.first.position == str.end());
}

TEST_CASE("until no eat include") {
    using namespace parsimon;
    constexpr auto parseNumber = parse_result(between_items('{', '}'), integer());
    constexpr auto p = until<false, true>(parseNumber);
    constexpr std::string_view str("abc{123}");

    constexpr auto res = p.parse(str);

    static_assert(res.second);
    static_assert(*res.second == "abc{123}");
    static_assert(res.first.position == str.begin() + 3);
}

TEST_CASE("until end") {
    using namespace parsimon;
    constexpr auto untilEnd = until(empty());
    constexpr std::string_view str("abc{123}");
    constexpr auto res = untilEnd.parse(str);
    static_assert(res.second);
    static_assert(*res.second == "abc{123}");
}

TEST_CASE("until end empty") {
    using namespace parsimon;
    constexpr auto untilEnd = until(empty());
    constexpr std::string_view str("");
    constexpr auto res = untilEnd.parse(str);
    static_assert(res.second);
    static_assert(*res.second == "");
}

TEST_CASE("many_f with separator") {
    using namespace parsimon;
    constexpr auto intParser = integer();

    int result = 0;
    auto p = many_f(intParser, [&result](auto i) {
        result += i;
    }, sequence("#%"));

    auto res = p.parse("100#%20#%3");

    REQUIRE(res.second);
    REQUIRE(*res.second == "100#%20#%3");
    REQUIRE(result == 123);
}

TEST_CASE("many_state with separator") {
    using namespace parsimon;
    constexpr auto intParser = integer();

    constexpr auto p = many_state(intParser, [](auto& s, auto i) {
        s += i;
    }, sequence("#%"));

    constexpr auto res = p.parse_with_state("100#%20#%3", 0);
    static_assert(res.second);
    static_assert(*res.second == "100#%20#%3");
    static_assert(res.first.user_state == 123);
}

TEST_CASE("recursive") {
    using namespace parsimon;
    constexpr std::string_view str("{{{{{{{{123}}}}}}}}");
    constexpr auto rec_parser = recursive<int>([](auto p) {
        return integer() || (item<'{'>() >> p << item<'}'>());
    });
    constexpr auto res = rec_parser.parse(str);
    static_assert(res.second);
    static_assert(*res.second == 123);
    static_assert(res.first.position == str.end());
}

TEST_CASE("chain") {
    using namespace parsimon;
    constexpr std::string_view str("8/2/2");

    constexpr auto op = item<'/'>() >= std::divides<>();

    constexpr auto parser = chain(integer(), op);

    constexpr auto res = parser.parse(str);
    static_assert(res.second);
    static_assert(*res.second == 2);
    static_assert(res.first.position == str.end());
}
