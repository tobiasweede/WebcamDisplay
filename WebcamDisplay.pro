#-------------------------------------------------
#
# Project created by QtCreator 2015-07-29T17:40:31
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = WebcamDisplay
TEMPLATE = app


SOURCES += src/main.cpp\
        src/webcamdisplay.cpp

HEADERS  += src/webcamdisplay.h

FORMS    += ui/webcamdisplay.ui \
    ui/authenticationdialog.ui

DISTFILES += \
    codingStyle.astylerc \

OTHER_FILES += \
    CHANGELOG.md \
    LICENSE \
    README.md \
    doc/doc.md
