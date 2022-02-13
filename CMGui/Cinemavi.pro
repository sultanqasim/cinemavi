QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../auto_exposure.c \
    ../cmraw.c \
    ../colour_xfrm.c \
    ../convolve.c \
    ../debayer.c \
    ../dng.cpp \
    ../gamma.c \
    ../noise_reduction.c \
    ../pipeline.c \
    cmnumberslider.cpp \
    cmpicturelabel.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ../auto_exposure.h \
    ../cmraw.h \
    ../colour_xfrm.h \
    ../convolve.h \
    ../debayer.h \
    ../dng.h \
    ../gamma.h \
    ../noise_reduction.h \
    ../pipeline.h \
    ../tiny_dng_writer.h \
    ../ycrcg.h \
    cmnumberslider.h \
    cmpicturelabel.h \
    mainwindow.h

FORMS +=

TRANSLATIONS += \
    Cinemavi_en_CA.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
