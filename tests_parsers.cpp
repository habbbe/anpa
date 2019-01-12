#include "test/catch.hpp"
#include "parser.h"

TEST_CASE("success") {
    REQUIRE(parse::success().parse("").second);
}

TEST_CASE("fail") {
    REQUIRE(!parse::fail().parse("").second);
}

TEST_CASE("empty") {
    REQUIRE(parse::empty().parse(std::string("")).second);
    REQUIRE(!parse::empty().parse(std::string(" ")).second);
}

TEST_CASE("any_item") {
    REQUIRE(*parse::any_item().parse(std::string("a")).second == 'a');
    REQUIRE(!parse::any_item().parse(std::string("")).second);
}

TEST_CASE("item") {
    REQUIRE(*parse::item('a').parse(std::string("a")).second == 'a');
    REQUIRE(!parse::item('b').parse(std::string("a")).second);
}

TEST_CASE("item_if") {
    constexpr auto p = parse::item_if([](auto &c) {return c == 'a';});
    constexpr auto res = p.parse(std::string_view("a"));
    REQUIRE(*res.second == 'a');
    REQUIRE(!p.parse(std::string("b")).second);
}

TEST_CASE("custom") {
    auto parser = [](auto begin, auto end) {
        using type = std::pair<std::decay_t<decltype(begin)>, std::optional<int>>;
        if (begin == end) {
            return type(end, {});
        } else if (*begin == 'a') {
            return type{++begin, 1};
        } else if (*begin == 'b') {
            return type{++begin, 2};
        } else {
            return type(begin, {});
        }
    };
    std::string strA("a");
    std::string strB("b");
    std::string strC("c");
    std::string strEmpty("");

    auto resA = parse::custom(parser).parse(strA);
    auto resB = parse::custom(parser).parse(strB);
    auto resC = parse::custom(parser).parse(strC);
    auto resEmpty = parse::custom(parser).parse(strEmpty);

    REQUIRE(*resA.second == 1);
    REQUIRE(resA.first == strA.begin() + 1);

    REQUIRE(*resB.second == 2);
    REQUIRE(resB.first == strB.begin() + 1);

    REQUIRE(!resC.second);
    REQUIRE(resC.first == strC.begin());

    REQUIRE(!resEmpty.second);
    REQUIRE(resEmpty.first == strEmpty.end());
}

TEST_CASE("custom_state") {
    auto parser = [](auto begin, auto, auto &state) {
        using type = std::pair<std::decay_t<decltype(begin)>, std::optional<int>>;
        state = 3;
        return type(begin, 3);
    };
    std::string str("a");
    int i = 0;
    auto res = parse::custom_with_state(parser).parse_with_state(str, i);
    REQUIRE(res.second);
    REQUIRE(*res.second == 3);
    REQUIRE(i == 3);
}

TEST_CASE("sequence") {
    std::string str("abcde");
    auto resSuccess = parse::sequence("abc").parse(str);
    REQUIRE(resSuccess.second);
    REQUIRE(*resSuccess.second == "abc");
    REQUIRE(resSuccess.first == str.begin() + 3);

    auto resParialFail = parse::sequence("abce").parse(str);
    REQUIRE(!resParialFail.second);
    REQUIRE(resParialFail.first == str.begin());

    auto resTooLong = parse::sequence("abcdef").parse(str);
    REQUIRE(!resTooLong.second);
    REQUIRE(resTooLong.first == str.begin());

    auto resFail = parse::sequence("b").parse(str);
    REQUIRE(!resFail.second);
    REQUIRE(resFail.first == str.begin());
}

TEST_CASE("consume") {
    std::string str("abcde");
    auto resSuccess = parse::consume(3).parse(str);
    REQUIRE(resSuccess.second);
    REQUIRE(*resSuccess.second == "abc");
    REQUIRE(resSuccess.first == str.begin() + 3);

    auto resFail = parse::consume(6).parse(str);
    REQUIRE(!resFail.second);
    REQUIRE(resFail.first == str.begin());
}

TEST_CASE("until_item eat no include") {
    std::string str("abcde");
    auto res = parse::until_item('c').parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "ab");
    REQUIRE(res.first == str.begin() + 3);
}

TEST_CASE("until_item no eat no include") {
    std::string str("abcde");
    auto res = parse::until_item<false, false>('c').parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "ab");
    REQUIRE(res.first == str.begin() + 2);
}

TEST_CASE("until_item eat include") {
    std::string str("abcde");
    auto res = parse::until_item<true, true>('c').parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abc");
    REQUIRE(res.first == str.begin() + 3);
}

TEST_CASE("until_item no eat include") {
    std::string str("abcde");
    auto res = parse::until_item<false, true>('c').parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abc");
    REQUIRE(res.first == str.begin() + 2);
}

TEST_CASE("until_sequence eat no include") {
    std::string str("abcde");
    auto res = parse::until_sequence("cd").parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "ab");
    REQUIRE(res.first == str.begin() + 4);

    auto resFail = parse::until_sequence("cdf").parse(str);
    REQUIRE(!resFail.second);
    REQUIRE(resFail.first == str.begin());
}

TEST_CASE("until_sequence no eat no include") {
    std::string str("abcde");
    auto res = parse::until_sequence<false, false>("cd").parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "ab");
    REQUIRE(res.first == str.begin() + 2);
}

TEST_CASE("until_sequence eat include") {
    std::string str("abcde");
    auto res = parse::until_sequence<true, true>("cd").parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcd");
    REQUIRE(res.first == str.begin() + 4);
}

TEST_CASE("until_sequence no eat include") {
    std::string str("abcde");
    auto res = parse::until_sequence<false, true>("cd").parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcd");
    REQUIRE(res.first == str.begin() + 2);
}

TEST_CASE("rest") {
    std::string str("abcde");
    auto res = parse::rest().parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
    REQUIRE(res.first == str.end());

    std::string strEmpty;
    auto resEmpty = parse::rest().parse(strEmpty);
    REQUIRE(resEmpty.second);
    REQUIRE(*resEmpty.second == "");
    REQUIRE(resEmpty.first == strEmpty.begin());
    REQUIRE(resEmpty.first == strEmpty.end());
}

TEST_CASE("while_predicate") {
    auto pred = [](auto &x) {
        return x == 'a' || x == 'b';
    };

    std::string str("aabbcc");
    auto res = parse::while_predicate(pred).parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "aabb");
    REQUIRE(res.first == str.begin() + 4);

    std::string strNoMatch("cbbaa");
    auto resNoMatch = parse::while_predicate(pred).parse(strNoMatch);
    REQUIRE(resNoMatch.second);
    REQUIRE(*resNoMatch.second == "");
    REQUIRE(resNoMatch.first == strNoMatch.begin());
}

TEST_CASE("while_in") {
    std::string str("aabbcc");
    auto res = parse::while_in("abc").parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "aabbcc");
    REQUIRE(res.first == str.end());

    auto resNoMatch = parse::while_in("def").parse(str);
    REQUIRE(resNoMatch.second);
    REQUIRE(*resNoMatch.second == "");
    REQUIRE(resNoMatch.first == str.begin());
}

TEST_CASE("between_sequences") {
    std::string str("beginabcdeend");
    auto res = parse::between_sequences("begin", "end").parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
    REQUIRE(res.first == str.end());

    auto resNoEat = parse::between_sequences<false, false>("begin", "end").parse(str);
    REQUIRE(resNoEat.second);
    REQUIRE(*resNoEat.second == "beginabcdeend");
    REQUIRE(resNoEat.first == str.end());
}

TEST_CASE("between_sequences nested") {
    std::string str("beginbeginabcdeendend");
    auto res = parse::between_sequences<true>("begin", "end").parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "beginabcdeend");
    REQUIRE(res.first == str.end());

    auto resNoEat = parse::between_sequences<true, false>("begin", "end").parse(str);
    REQUIRE(resNoEat.second);
    REQUIRE(*resNoEat.second == "beginbeginabcdeendend");
    REQUIRE(resNoEat.first == str.end());

    std::string strNonClosing("beginbeginabcdeend");
    auto resNonClosing = parse::between_sequences<true>("begin", "end").parse(strNonClosing);
    REQUIRE(!resNonClosing.second);
    REQUIRE(resNonClosing.first == strNonClosing.begin());
}

TEST_CASE("between_items") {
    std::string str("{abcde}");
    auto res = parse::between_items('{', '}').parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == "abcde");
    REQUIRE(res.first == str.end());
}

TEST_CASE("integer") {
    constexpr std::string_view str("42abcde");
    constexpr auto res = parse::integer().parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == 42);
    REQUIRE(res.first == str.begin() + 2);

    std::string str2("-42abcde");
    auto res2 = parse::integer().parse(str2);
    REQUIRE(res2.second);
    REQUIRE(*res2.second == -42);
    REQUIRE(res2.first == str2.begin() + 3);

    std::string str3("-42abcde");
    auto res3 = parse::integer<unsigned int>().parse(str3);
    REQUIRE(!res3.second);
    REQUIRE(res3.first == str3.begin());

    std::string str4("42abcde");
    auto res4 = parse::integer<unsigned int>().parse(str4);
    REQUIRE(res4.second);
    REQUIRE(*res4.second == 42);
    REQUIRE(res4.first == str4.begin() + 2);
}

//TEST_CASE("floating") {
//    auto p = parse::floating();
//    auto test = [&](auto s, double r) {
//        std::string str(s);
//        auto res = p.parse(str);
//        REQUIRE(res.second);
//        REQUIRE(*res.second == r);
//        REQUIRE(res.first == str.end());
//    };

//    test("123", 123.0);
//    test("-123", -123.0);
//    test("123.321", 123.321);
//    test("-123.321", -123.321);
//    test("123.0", 123.0);
//    test("-123.0", -123.0);
//    test("123e3", 123e3);
//    test("-123e3", -123e3);
//    test("123e-3", 123e-3);
//    test("-123e-3", -123e-3);
//    test("123.321e3", 123.321e3);
//    test("-123.321e3", -123.321e3);
//    test("123.321e-3", 123.321e-3);
//    test("-123.321e-3", -123.321e-3);
//}

TEST_CASE("number") {
    auto &str = "42abcde";

    auto res = parse::number<int>().parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == 42);
    REQUIRE(res.first == str + 2);

    // Doesn't seem to compile?
//    auto &str2 = "42.3abcde";
//    auto res2 = parse::number<float>().parse(str2);
//    REQUIRE(res2.second);
//    REQUIRE(*res2.second == 42.3);
//    REQUIRE(res2.first == str2 + 4);
}
