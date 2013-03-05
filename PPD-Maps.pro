#-------------------------------------------------
#
# Project created by QtCreator 2012-07-12T14:33:34
#
#-------------------------------------------------

QT       += core gui opengl

TARGET = PPD-Maps
TEMPLATE = app

win32:RC_FILE = winicon.rc

VERSION = 1.3.2

SVN_REV = $$system(svnversion)
#SVN_REV = 1
VERSION_STR = '\\"$${VERSION}\\"'
SVN_REV_STR = '\\"$${SVN_REV}\\"'

DEFINES += APP_VERSION=\"$${VERSION_STR}\"
DEFINES += APP_SVN_REV=\"$${SVN_REV_STR}\"

SOURCES += main.cpp\
        mainwindow.cpp \
    axis.cpp \
    map.cpp \
    mapsearch.cpp \
    mapscale.cpp \
    ppddata.cpp \
    maptablemodel.cpp \
    about.cpp \
    exportcreator.cpp \
    mappreset.cpp \
    conversion.cpp \
    clearablelineedit.cpp \
    util.cpp

HEADERS  += mainwindow.h \
    axis.h \
    map.h \
    mapsearch.h \
    mapscale.h \
    ppddata.h \
    maptablemodel.h \
    about.h \
    exportcreator.h \
    mappreset.h \
    conversion.h \
    clearablelineedit.h \
    util.h

FORMS    += mainwindow.ui \
    about.ui

CONFIG(release, debug|release) {
    linux-g++:LIBS += -lqwtplot3d-qt4 -lGLU
    linux-g++:INCLUDEPATH += /usr/include/qwtplot3d-qt4/
    win32:LIBS += -lqwtplot3d -L../qwtplot3d/lib
    win32:INCLUDEPATH += "../qwtplot3d/include"
}
else {
    LIBS += -lqwtplot3d -L../qwtplot3d/lib
    INCLUDEPATH += "../qwtplot3d/include"
}

RESOURCES += \
    resource.qrc

OTHER_FILES += \
    winicon.rc
