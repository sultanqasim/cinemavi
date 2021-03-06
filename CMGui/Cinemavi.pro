QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Aravis
macx {
    LIBS += -L/opt/homebrew/lib -laravis-0.8 -lgobject-2.0
    INCLUDEPATH += /opt/homebrew/include/aravis-0.8
    INCLUDEPATH += /opt/homebrew/include/glib-2.0
    INCLUDEPATH += /opt/homebrew/lib/glib-2.0/include
}
linux {
    LIBS += -laravis-0.8 -lgobject-2.0
    INCLUDEPATH += /usr/include/aravis-0.8
    INCLUDEPATH += /usr/include/glib-2.0
    INCLUDEPATH += /usr/lib/glib-2.0/include
}

SOURCES += \
    ../auto_exposure.c \
    ../cm_calibrations.c \
    ../cm_camera_helper.c \
    ../cmraw.c \
    ../colour_xfrm.c \
    ../convolve.c \
    ../debayer.c \
    ../dng.cpp \
    ../gamma.c \
    ../noise_reduction.c \
    ../pipeline.c \
    cmautoexposure.cpp \
    cmcameracontrols.cpp \
    cmcamerainterface.cpp \
    cmcontrolswidget.cpp \
    cmnumberslider.cpp \
    cmpicturelabel.cpp \
    cmrawimage.cpp \
    cmrawinfowidget.cpp \
    cmrenderqueue.cpp \
    cmrenderworker.cpp \
    cmsaveworker.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ../auto_exposure.h \
    ../cie_xyz.h \
    ../cm_calibrations.h \
    ../cm_camera_helper.h \
    ../cmraw.h \
    ../colour_xfrm.h \
    ../convolve.h \
    ../debayer.h \
    ../dng.h \
    ../gamma.h \
    ../noise_reduction.h \
    ../pipeline.h \
    ../tiny_dng_writer.h \
    ../ycbcr.h \
    ../ycrcg.h \
    cmautoexposure.h \
    cmcameracontrols.h \
    cmcamerainterface.h \
    cmcontrolswidget.h \
    cmnumberslider.h \
    cmpicturelabel.h \
    cmrawimage.h \
    cmrawinfowidget.h \
    cmrenderqueue.h \
    cmrenderworker.h \
    cmsaveworker.h \
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
