#include <catch2/catch.hpp>
#include <fstream>
#include <iostream>
#include "time_measure.h"
#include "calc.h"

TEST_CASE("plus") {
    static_assert (expr.parse("8+2+2").second.get_value() == 12);
}

TEST_CASE("minus") {
    static_assert (expr.parse("8-2-2").second.get_value() == 4);
}

TEST_CASE("times") {
    static_assert (expr.parse("8*2*2").second.get_value() == 32);
}

TEST_CASE("div") {
    static_assert (expr.parse("8/2/2").second.get_value() == 2);
}

TEST_CASE("parens") {
    static_assert (expr.parse("2*(3+1)").second.get_value() == 8);
}

TEST_CASE("precedence") {
    static_assert (expr.parse("4*2/2+1-5*2").second.get_value() == -5);
}

TEST_CASE("all together") {
    static_assert (expr.parse("4*2/2+(1-5)*2").second.get_value() == -4);
}
