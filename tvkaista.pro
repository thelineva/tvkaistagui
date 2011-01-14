# -------------------------------------------------
# Project created by QtCreator 2010-12-14T11:39:12
# -------------------------------------------------
QT += core \
    gui \
    xml \
    network
TARGET = tvkaistagui
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    tvkaistaclient.cpp \
    channelfeedparser.cpp \
    channel.cpp \
    programme.cpp \
    programmetableparser.cpp \
    htmlparser.cpp \
    cache.cpp \
    programmetablemodel.cpp \
    settingsdialog.cpp \
    downloader.cpp \
    downloadtablemodel.cpp \
    downloaddelegate.cpp \
    programmefeedparser.cpp \
    aboutdialog.cpp \
    screenshotwindow.cpp \
    thumbnail.cpp
HEADERS += mainwindow.h \
    tvkaistaclient.h \
    channelfeedparser.h \
    channel.h \
    programme.h \
    programmetableparser.h \
    htmlparser.h \
    cache.h \
    programmetablemodel.h \
    settingsdialog.h \
    downloader.h \
    downloadtablemodel.h \
    downloaddelegate.h \
    programmefeedparser.h \
    aboutdialog.h \
    screenshotwindow.h \
    thumbnail.h
FORMS += mainwindow.ui \
    settingsdialog.ui \
    aboutdialog.ui \
    screenshotwindow.ui
RESOURCES += images.qrc
RC_FILE = tvkaista.rc
TRANSLATIONS = translations/qt_fi.ts
VERSION = 1.0.1
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
unix:DEFINES += TVKAISTAGUI_TRANSLATIONS_DIR=\\\"/usr/share/tvkaistagui/translations\\\"
