#include <catch2/catch.hpp>
#include "parser.h"

TEST_CASE("success") {
    static_assert(parse::success().parse("").second);
}

TEST_CASE("fail") {
    static_assert(!parse::fail().parse("").second);
}

TEST_CASE("empty") {
    static_assert(parse::empty().parse("").second);
    static_assert(!parse::empty().parse(" ").second);
}

TEST_CASE("any_item") {
    static_assert(*parse::any_item().parse("a").second == 'a');
    static_assert(!parse::any_item().parse("").second);
}

TEST_CASE("item") {
    static_assert(*parse::item('a').parse("a").second == 'a');
    static_assert(!parse::item('b').parse("a").second);
}

TEST_CASE("item_if") {
    constexpr std::string_view str("abc");
    constexpr auto p = parse::item_if([](auto& c) {return c == 'a';});
    constexpr auto res = p.parse(str);
    static_assert(*res.second == 'a');
    static_assert(res.first.position == str.begin() + 1);
    constexpr std::string_view strFail("bbc");
    constexpr auto resFail = p.parse(strFail);
    static_assert(!resFail.second);
    static_assert(resFail.first.position == strFail.begin());
}

TEST_CASE("custom") {
    constexpr auto parser = [](auto begin, auto end) {
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
    constexpr std::string_view strA("a");
    constexpr std::string_view strB("b");
    constexpr std::string_view strC("c");
    constexpr std::string_view strEmpty("");

    constexpr auto resA = parse::custom(parser).parse(strA);
    constexpr auto resB = parse::custom(parser).parse(strB);
    constexpr auto resC = parse::custom(parser).parse(strC);
    constexpr auto resEmpty = parse::custom(parser).parse(strEmpty);

    static_assert(*resA.second == 1);
    static_assert(resA.first.position == strA.begin() + 1);

    static_assert(*resB.second == 2);
    static_assert(resB.first.position == strB.begin() + 1);

    static_assert(!resC.second);
    static_assert(resC.first.position == strC.begin());

    static_assert(!resEmpty.second);
    static_assert(resEmpty.first.position == strEmpty.end());
}

TEST_CASE("custom_state") {
    constexpr auto parser = [](auto begin, auto, auto& state) {
        using type = std::pair<std::decay_t<decltype(begin)>, std::optional<int>>;
        state = 3;
        return type(begin, 3);
    };
    constexpr std::string_view str("a");
    constexpr auto res = parse::custom_with_state(parser).parse_with_state(str, 0);
    static_assert(res.second);
    static_assert(*res.second == 3);
    static_assert(res.first.user_state == 3);
}

TEST_CASE("sequence") {
    constexpr std::string_view str("abcde");
    constexpr auto resSuccess = parse::sequence("abc").parse(str);
    static_assert(resSuccess.second);
    static_assert(*resSuccess.second == "abc");
    static_assert(resSuccess.first.position == str.begin() + 3);

    constexpr auto resPartialFail = parse::sequence("abce").parse(str);
    static_assert(!resPartialFail.second);
    static_assert(resPartialFail.first.position == str.begin());

    constexpr auto resTooLong = parse::sequence("abcdef").parse(str);
    static_assert(!resTooLong.second);
    static_assert(resTooLong.first.position == str.begin());

    constexpr auto resFail = parse::sequence("b").parse(str);
    static_assert(!resFail.second);
    static_assert(resFail.first.position == str.begin());
}

TEST_CASE("consume") {
    constexpr std::string_view str("abcde");
    constexpr auto resSuccess = parse::consume(3).parse(str);
    static_assert(resSuccess.second);
    static_assert(*resSuccess.second == "abc");
    static_assert(resSuccess.first.position == str.begin() + 3);

    constexpr auto resFail = parse::consume(6).parse(str);
    static_assert(!resFail.second);
    static_assert(resFail.first.position == str.begin());
}

TEST_CASE("until_item eat no include") {
    constexpr std::string_view str("abcde");
    constexpr auto res = parse::until_item('c').parse(str);
    static_assert(res.second);
    static_assert(*res.second == "ab");
    static_assert(res.first.position == str.begin() + 3);
}

TEST_CASE("until_item no eat no include") {
    constexpr std::string_view str("abcde");
    constexpr auto res = parse::until_item<false, false>('c').parse(str);
    static_assert(res.second);
    static_assert(*res.second == "ab");
    static_assert(res.first.position == str.begin() + 2);
}

TEST_CASE("until_item eat include") {
    constexpr std::string_view str("abcde");
    constexpr auto res = parse::until_item<true, true>('c').parse(str);
    static_assert(res.second);
    static_assert(*res.second == "abc");
    static_assert(res.first.position == str.begin() + 3);
}

TEST_CASE("until_item no eat include") {
    constexpr std::string_view str("abcde");
    constexpr auto res = parse::until_item<false, true>('c').parse(str);
    static_assert(res.second);
    static_assert(*res.second == "abc");
    static_assert(res.first.position == str.begin() + 2);
}

TEST_CASE("until_sequence eat no include") {
    constexpr std::string_view str("abcde");
    constexpr auto res = parse::until_sequence("cd").parse(str);
    static_assert(res.second);
    static_assert(*res.second == "ab");
    static_assert(res.first.position == str.begin() + 4);

    constexpr auto resFail = parse::until_sequence("cdf").parse(str);
    static_assert(!resFail.second);
    static_assert(resFail.first.position == str.begin());
}

TEST_CASE("until_sequence no eat no include") {
    constexpr std::string_view str("abcde");
    constexpr auto res = parse::until_sequence<false, false>("cd").parse(str);
    static_assert(res.second);
    static_assert(*res.second == "ab");
    static_assert(res.first.position == str.begin() + 2);
}

TEST_CASE("until_sequence eat include") {
    constexpr std::string_view str("abcde");
    constexpr auto res = parse::until_sequence<true, true>("cd").parse(str);
    static_assert(res.second);
    static_assert(*res.second == "abcd");
    static_assert(res.first.position == str.begin() + 4);
}

TEST_CASE("until_sequence no eat include") {
    constexpr std::string_view str("abcde");
    constexpr auto res = parse::until_sequence<false, true>("cd").parse(str);
    static_assert(res.second);
    static_assert(*res.second == "abcd");
    static_assert(res.first.position == str.begin() + 2);
}

TEST_CASE("rest") {
    constexpr std::string_view str("abcde");
    constexpr auto res = parse::rest().parse(str);
    static_assert(res.second);
    static_assert(*res.second == "abcde");
    static_assert(res.first.position == str.end());

    constexpr std::string_view strEmpty;
    constexpr auto resEmpty = parse::rest().parse(strEmpty);
    static_assert(resEmpty.second);
    static_assert(*resEmpty.second == "");
    static_assert(resEmpty.first.position == strEmpty.begin());
    static_assert(resEmpty.first.position == strEmpty.end());
}

TEST_CASE("while_predicate") {
    auto pred = [](auto& x) {
        return x == 'a' || x == 'b';
    };

    constexpr std::string_view str("aabbcc");
    constexpr auto res = parse::while_predicate(pred).parse(str);
    static_assert(res.second);
    static_assert(*res.second == "aabb");
    static_assert(res.first.position == str.begin() + 4);

    constexpr std::string_view strNoMatch("cbbaa");
    constexpr auto resNoMatch = parse::while_predicate(pred).parse(strNoMatch);
    static_assert(resNoMatch.second);
    static_assert(*resNoMatch.second == "");
    static_assert(resNoMatch.first.position == strNoMatch.begin());
}

TEST_CASE("while_in") {
    constexpr std::string_view str("aabbcc");
    constexpr auto res = parse::while_in("abc").parse(str);
    static_assert(res.second);
    static_assert(*res.second == "aabbcc");
    static_assert(res.first.position == str.end());

    constexpr auto resNoMatch = parse::while_in("def").parse(str);
    static_assert(resNoMatch.second);
    static_assert(*resNoMatch.second == "");
    static_assert(resNoMatch.first.position == str.begin());
}

TEST_CASE("between_sequences") {
    constexpr std::string_view str("beginabcdeend");
    constexpr auto res = parse::between_sequences("begin", "end").parse(str);
    static_assert(res.second);
    static_assert(*res.second == "abcde");
    static_assert(res.first.position == str.end());

    constexpr auto resNoEat = parse::between_sequences<false, false>("begin", "end").parse(str);
    static_assert(resNoEat.second);
    static_assert(*resNoEat.second == "beginabcdeend");
    static_assert(resNoEat.first.position == str.end());
}

TEST_CASE("between_sequences nested") {
    constexpr std::string_view str("beginbeginabcdeendend");
    constexpr auto res = parse::between_sequences<true>("begin", "end").parse(str);
    static_assert(res.second);
    static_assert(*res.second == "beginabcdeend");
    static_assert(res.first.position == str.end());

    constexpr auto resNoEat = parse::between_sequences<true, false>("begin", "end").parse(str);
    static_assert(resNoEat.second);
    static_assert(*resNoEat.second == "beginbeginabcdeendend");
    static_assert(resNoEat.first.position == str.end());

    constexpr std::string_view strNonClosing("beginbeginabcdeend");
    constexpr auto resNonClosing = parse::between_sequences<true>("begin", "end").parse(strNonClosing);
    static_assert(!resNonClosing.second);
    static_assert(resNonClosing.first.position == strNonClosing.begin());
}

TEST_CASE("between_items") {
    constexpr std::string_view str("{abcde}");
    constexpr auto res = parse::between_items('{', '}').parse(str);
    static_assert(res.second);
    static_assert(*res.second == "abcde");
    static_assert(res.first.position == str.end());
}

TEST_CASE("integer") {
    constexpr std::string_view str("42abcde");
    constexpr auto res = parse::integer().parse(str);
    static_assert(res.second);
    static_assert(*res.second == 42);
    static_assert(res.first.position == str.begin() + 2);

    constexpr std::string_view str2("-42abcde");
    constexpr auto res2 = parse::integer().parse(str2);
    static_assert(res2.second);
    static_assert(*res2.second == -42);
    static_assert(res2.first.position == str2.begin() + 3);

    constexpr std::string_view str3("-42abcde");
    constexpr auto res3 = parse::integer<unsigned int>().parse(str3);
    static_assert(!res3.second);
    static_assert(res3.first.position == str3.begin());

    constexpr std::string_view str4("42abcde");
    constexpr auto res4 = parse::integer<unsigned int>().parse(str4);
    static_assert(res4.second);
    static_assert(*res4.second == 42);
    static_assert(res4.first.position == str4.begin() + 2);
}

TEST_CASE("floating") {
    constexpr auto p = parse::floating();
#define floating_test_(s,b) { constexpr std::string_view str(s); \
        constexpr auto res = p.parse(str); \
        static_assert(res.second); \
        static_assert(*res.second == b); \
        static_assert(res.first.position == str.end());}
    floating_test_("123", 123.0);
    floating_test_("-123", -123.0);
    floating_test_("123.321", 123.321);
    floating_test_("-123.321", -123.321);
    floating_test_("123.0", 123.0);
    floating_test_("-123.0", -123.0);
    floating_test_("123e1", 1230);
    floating_test_("123e3", 123e3);
    floating_test_("-123e3", -123e3);
    floating_test_("123e-3", 123e-3);
    floating_test_("-123e-3", -123e-3);
    floating_test_("123.321e3", 123.321e3);
    floating_test_("-123.321e3", -123.321e3);
    floating_test_("123.321e-3", 123.321e-3);
    floating_test_("-123.321e-3", -123.321e-3);
}

TEST_CASE("number") {
    auto& str = "42abcde";
    auto res = parse::number<int>().parse(str);
    REQUIRE(res.second);
    REQUIRE(*res.second == 42);
    REQUIRE(res.first.position == str + 2);

    // No support for floats yet.
//    auto& str2 = "42.3abcde";
//    constexpr auto res2 = parse::number<float>().parse(str2);
//    static_assert(res2.second);
//    static_assert(*res2.second == 42.3);
//    static_assert(res2.first.position == str2 + 4);
}
