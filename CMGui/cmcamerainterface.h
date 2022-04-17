#ifndef CMCAMERAINTERFACE_H
#define CMCAMERAINTERFACE_H

#include <arv.h>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QString>
#include "cmrawimage.h"
#include "cmautoexposure.h"
#include "../auto_exposure.h"

class CMCameraInterface : public QObject
{
    Q_OBJECT
public:
    explicit CMCameraInterface();
    ~CMCameraInterface();

    bool cameraAvailable();
    void setFrameRate(double frameRate);
    void setExposure(double shutter_us, double gain_dB);
    void getExposure(double *shutter_us, double *gain_dB);
    void updateExposure(CMExposureMode expMode, double changeFactor);
    ExposureLimits & getExposureLimits();
    void startCapture();
    void stopCapture();
    const std::string & getCameraMake();
    const std::string & getCameraModel();
    QString getCameraName();

signals:
    void imageCaptured(CMRawImage &img);

private slots:
    void captureLoop();

private:
    QThread captureThread;
    ArvCamera *camera = NULL;
    ArvStream *stream;
    std::string cameraMake;
    std::string cameraModel;
    volatile bool capturing = false;
    ExposureLimits expLimits;
    double frameRateMin;
    double frameRateMax;
    double shutter = 0;
    double gain = 0;
};

#endif // CMCAMERAINTERFACE_H
