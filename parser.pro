TEMPLATE = app
CONFIG += console c++1z
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3

SOURCES += main.cpp

HEADERS += \
    parser.h \
    monad.h \
    lazy.h \
    parser_core.h \
    parser_combinators.h \
    parser_parsers.h
