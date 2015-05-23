TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    pak.c \
    util.c \
    dump_pak.c \
    make_pak.c \
    print_pak_info.c
QMAKE_CFLAGS += -std=c11

LIBS += -lz
DEFINES += __USE_LARGEFILE64 _LARGEFILE_SOURCE _LARGEFILE64_SOURCE

HEADERS += \
    pak.h \
    util.h

