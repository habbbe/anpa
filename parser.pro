TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++17
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3

SOURCES += \
    tests_main.cpp \
    tests_parsers.cpp \
    tests_combinators.cpp \
    tests_perf.cpp \
    tests_json.cpp

HEADERS += \
    parser.h \
    monad.h \
    lazy.h \
    parser_core.h \
    parser_combinators.h \
    parser_parsers.h \
    json_parser.h \
    parser_result.h \
    parser_state.h \
    parser_settings.h \
    parse_algorithm.h \
    time_measure.h \
    json_parser.h \
    lazy.h \
    monad.h \
    parse_algorithm.h \
    parser.h \
    parser_combinators.h \
    parser_core.h \
    parser_parsers.h \
    parser_result.h \
    parser_settings.h \
    parser_state.h \
    time_measure.h \
    test/catch.hpp \
    json_parser.h \
    lazy.h \
    monad.h \
    parse_algorithm.h \
    parser.h \
    parser_combinators.h \
    parser_core.h \
    parser_parsers.h \
    parser_result.h \
    parser_settings.h \
    parser_state.h \
    time_measure.h \
    parser_types.h \
    json/json_value.h \
    json/json_parser.h \
    parser_parsers_internal.h \
    parser_combinators_internal.h
