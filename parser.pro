TEMPLATE = app
CONFIG += console c++1z
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O2

SOURCES += main.cpp

HEADERS += \
    parser.h \
    monad.h \
    functor.h \
    lazy.h
