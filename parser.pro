TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
#CONFIG += force_debug_info
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
    lazy.h \
    json_parser.h \
    time_measure.h \
    json_parser.h \
    lazy.h \
    parser.h \
    time_measure.h \
    test/catch.hpp \
    json_parser.h \
    lazy.h \
    parser.h \
    time_measure.h \
    json/json_value.h \
    json/json_parser.h \
    algorithm.h \
    combinators.h \
    core.h \
    monad.h \
    parsers.h \
    combinators_internal.h \
    result.h \
    settings.h \
    state.h \
    types.h \
    parsers_internal.h
