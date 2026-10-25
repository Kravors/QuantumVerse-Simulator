QT       += core
QT       -= gui
CONFIG   += console c++17
CONFIG   -= app_bundle
TEMPLATE  = app
TARGET    = paper_harness

INCLUDEPATH += $$PWD/src/discovery
INCLUDEPATH += $$PWD/src

SOURCES += \
    harness_main.cpp \
    src/discovery/PaperGenerator.cpp

HEADERS += \
    src/discovery/PaperGenerator.h

RESOURCES += \
    harness.qrc
