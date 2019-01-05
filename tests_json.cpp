#include <fstream>
#include "test/catch.hpp"
#include "json/json_parser.h"
#include "parser_state.h"
#include "parser_settings.h"
#include "time_measure.h"

template <typename T, typename Str>
auto test_json_type(Str &&s, T val) {
    std::string_view str(s);
    auto p = json_parser;
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
    test_json_type<json_string>("\"abc\"", "abc");
}

TEST_CASE("json_general") {
    std::string_view str("{\"first\"  :  [3e5 ,[ \"cba\" ,null], {\"ef\":false}], \"second\":true}");
    auto res = json_parser.parse(str);
    REQUIRE(res.second);
    auto &json = *res.second;
    REQUIRE(json.is_a<json_object>());

    REQUIRE(json.contains("first"));

    auto &element1 = json.at("first");

    REQUIRE(element1.is_a<json_array>());

    REQUIRE(element1.size() == 3);

    REQUIRE(element1[0].is_a<double>());
    REQUIRE(element1[0].get<double>() == 3e5);

    REQUIRE(element1[1].is_a<json_array>());
    REQUIRE(element1[1].size() == 2);
    REQUIRE(element1[1][0].is_a<json_string>());
    REQUIRE(element1[1][0].get<json_string>() == "cba");
    REQUIRE(element1[1][1].is_a<std::nullptr_t>());
    REQUIRE(element1[1][1].get<std::nullptr_t>() == nullptr);

    REQUIRE(element1[2].is_a<json_object>());
    auto object = element1[2];
    REQUIRE(object.size() == 1);
    REQUIRE(object.contains("ef"));
    REQUIRE(object.at("ef").is_a<bool>());
    REQUIRE(object.at("ef").get<bool>() == false);


    REQUIRE(json.contains("second"));
    REQUIRE(json.at("second").is_a<bool>());
    REQUIRE(json.at("second").get<bool>() == true);
}

TEST_CASE("performance_json") {
    std::ifstream t("canada.json");
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());

    TICK
    auto res = json_parser.parse(str);
    TOCK
    if (res.second) {
        std::cout << res.second->size() << std::endl;
    } else {
        std::cout << "No parse" << std::endl;
    }
}
