QT += core gui widgets network
CONFIG += c++17

CONFIG += static

TEMPLATE = app
TARGET = QtVncServer

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    vncserver.cpp

HEADERS += \
    mainwindow.h \
    vncserver.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

QTPLUGIN += qxcb qminimal

# -----------------------------
# Linux / X11 build (Kubuntu)
# -----------------------------
unix:!win32 {
    CONFIG += link_pkgconfig

    # libvncserver + the X11 libs needed by your code:
    # - XFlush is in x11
    # - XShm is in xext
    # - XTest is in xtst
    PKGCONFIG += libvncserver x11 xext xtst

    # (Optional but harmless; some setups want explicit pthread flags)
    QMAKE_CXXFLAGS += -pthread
    QMAKE_LFLAGS += -pthread
}


# -----------------------------
# Windows build (MXE)
# -----------------------------
win32 {
    MXE    = /home/john/dev/mxe
    TRIP   = x86_64-w64-mingw32.static
    PREFIX = $$MXE/usr/$$TRIP

    INCLUDEPATH += $$PREFIX/include
    LIBS += -L$$PREFIX/lib \
            -lvncserver -llzo2 -lz \
            -lssl -lcrypto \
            -lws2_32 -luser32 -lgdi32 -liphlpapi

    RC_FILE = appicon.rc
    DISTFILES += appicon.rc
}

DISTFILES +=
