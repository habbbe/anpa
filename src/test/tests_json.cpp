#include <fstream>
#include <catch2/catch.hpp>
#include "json/json_parser.h"
#include "time_measure.h"

template <typename T, typename Str>
auto test_json_type(Str&& s, T val) {
    std::string_view str(s);
    auto p = json_parser;
    auto res = p.parse(str);
    REQUIRE(res.second);
    REQUIRE(res.second->is_a<T>());
    REQUIRE(res.second->get<T>() == val);
}

TEST_CASE("json_number") {
    test_json_type<json_number>("-123.20", -123.20);
}

TEST_CASE("json_null") {
    test_json_type<json_null>("null", json_null());
}

TEST_CASE("json_bool") {
    test_json_type<bool>("true", true);
    test_json_type<bool>("false", false);
}

TEST_CASE("json_string") {
    test_json_type<json_string>("\"abc\"", "abc");
    REQUIRE(!json_parser.parse("\"abc").second);
}

TEST_CASE("json_general") {
    std::string_view str("{\"first\"  :  [3e5 ,[ \"cba\" ,null], {\"ef\":false}], \"second\":true}");
    auto res = json_parser.parse(str);
    REQUIRE(res.second);
    auto& json = *res.second;
    REQUIRE(json.is_a<json_object>());

    REQUIRE(json.contains("first"));

    auto& element1 = json.at("first");

    REQUIRE(element1.is_a<json_array>());

    REQUIRE(element1.size() == 3);

    REQUIRE(element1[0].is_a<json_number>());
    REQUIRE(element1[0].get<json_number>() == 3e5);

    REQUIRE(element1[1].is_a<json_array>());
    REQUIRE(element1[1].size() == 2);
    REQUIRE(element1[1][0].is_a<json_string>());
    REQUIRE(element1[1][0].get<json_string>() == "cba");
    REQUIRE(element1[1][1].is_a<json_null>());
    REQUIRE(element1[1][1].get<json_null>() == json_null());

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
    std::ifstream t1("test_input/canada.json");
    std::string str1((std::istreambuf_iterator<char>(t1)),
                     std::istreambuf_iterator<char>());

    TICK;
    auto res1 = json_parser.parse(str1);
    TOCK("json");
    if (res1.second) {
        std::cout << res1.second->size() << std::endl;
    } else {
        std::cout << "No parse canada.json" << std::endl;
    }
}

