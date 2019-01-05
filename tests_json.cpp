#include "test/catch.hpp"
#include "json_parser.h"
#include "parser_state.h"
#include "parser_settings.h"

template <typename T, typename Str>
auto test_json_type(Str &&s, T val) {
    std::string_view str(s);
    auto p = value_parser;
    auto res = p.parse(str);
    REQUIRE(res.second);
    REQUIRE(res.second->is_a<T>());
    REQUIRE(res.second->get<T>() == val);
}

TEST_CASE("json_number") {
    test_json_type<double>("-123.20", -123.20);
}

TEST_CASE("json_null") {
    test_json_type<std::nullptr_t>("null", nullptr);
}

TEST_CASE("json_bool") {
    test_json_type<bool>("true", true);
    test_json_type<bool>("false", false);
}

TEST_CASE("json_string") {
    test_json_type<std::string>("\"abc\"", "abc");
}

TEST_CASE("json_general") {
    std::string_view str("{\"abc\"  :  [3e5 ,[ \"cba\" ,null], {\"ef\":false}], \"second\":true}");
    auto res = value_parser.parse(str);
    REQUIRE(res.second);
    auto &json = *res.second;
    REQUIRE(json.is_a<json_object>());

    REQUIRE(json.contains("abc"));

//    auto &element1 = json["abc"];

//    REQUIRE(element1.is_a<json_array>());

//    REQUIRE(element1.get<json_array>().size() == 3);

//    REQUIRE(element1[0].is_a<double>());
//    REQUIRE(element1[0].get<double>() == 3e5);

}
