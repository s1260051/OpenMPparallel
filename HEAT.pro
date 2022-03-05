#-------------------------------------------------
#
# Project created by QtCreator 2015-11-26T11:51:51
#
#-------------------------------------------------

QT       += core gui sql opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

//!host_build:QMAKE_MAC_SDK = macosx10.10

TARGET = HEAT
TEMPLATE = app
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.10


//QMAKE_CXX = g++-4.9
//QMAKE_CC = gcc-4.9

//QMAKE_LINK = g++-4.9
//QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -std=c++11

QT += concurrent



SOURCES += main.cpp\
        mainwindow.cpp \
        rendering.cpp \
        showimage.cpp \
        popwindow.cpp \
        tiling.cpp \
    controlpanel.cpp \
    database.cpp \
    targetmodel.cpp \
    loaddatalist.cpp \
    vtkrendering.cpp \
    vtkmodel.cpp \
    showdbinfo.cpp \
    qcustomplot.cpp \
    pixcelgraph.cpp \
    calibration.cpp \
    calibrationgraph.cpp \
    controlgraphpanel.cpp \
    showfitsinfo.cpp \
    dailychange.cpp


HEADERS  += mainwindow.h \
    rendering.h \
    showimage.h \
    popwindow.h \
    tiling.h \
    controlpanel.h \
    database.h \
    targetmodel.h \
    loaddatalist.h \
    vtkrendering.h \
    vtkmodel.h \
    showdbinfo.h \
    qcustomplot.h \
    pixcelgraph.h \
    calibration.h \
    calibrationgraph.h \
    controlgraphpanel.h \
    showfitsinfo.h \
    omp.h \
    dailychange.h


FORMS    += mainwindow.ui \
    popwindow.ui \
    tiling.ui \
    controlpanel.ui \
    database.ui \
    targetmodel.ui \
    loaddatalist.ui \
    vtkmodel.ui \
    showdbinfo.ui \
    pkghkgraph.ui \
    pixcelgraph.ui \
    calibration.ui \
    calibrationgraph.ui \
    controlgraphpanel.ui \
    showfitsinfo.ui \
    dailychange.ui


QT += widgets

CONFIG += opengl console

INCLUDEPATH +=
INCLUDEPATH += /usr/local/include
## INCLUDEPATH += /usr/local/lib
INCLUDEPATH += /usr/local/Cellar/cfitsio/3.490/include
INCLUDEPATH += /usr/local/Cellar/ccfits/2.5_2/include/CCfits
INCLUDEPATH += /usr/local/Cellar/cspice/66/include
### success bigsur
### INCLUDEPATH += /usr/local/Cellar/cfitsio/3.470/include
### INCLUDEPATH += /usr/local/Cellar/ccfits/2.5_1/include/CCfits
### INCLUDEPATH += /usr/local/Cellar/cspice/66/include


DISTFILES +=
LIBS += -L/usr/local/Cellar/ccfits/2.5_2/lib -lCCfits
LIBS += -L/usr/local/Cellar/cfitsio/3.490/lib -lcfitsio
LIBS += /usr/local/Cellar/cspice/66/lib/cspice.a

QMAKE_CXXFLAGS += -Xpreprocessor -fopenmp -lomp -I/usr/local/include

QMAKE_LFLAGS += -lomp

LIBS += -L /usr/local/lib /usr/local/lib/libomp.dylib
