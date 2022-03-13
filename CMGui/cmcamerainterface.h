#ifndef CMCAMERAINTERFACE_H
#define CMCAMERAINTERFACE_H

#include <arv.h>
#include <QObject>
#include <QThread>
#include <QMutex>
#include "cmrawimage.h"
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
    void startCapture();
    void stopCapture();

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
    bool capturing = false;
    double shutterMin;
    double shutterMax;
    double gainMin;
    double gainMax;
    double frameRateMin;
    double frameRateMax;
    double shutter = 0;
    double gain = 0;
};

#endif // CMCAMERAINTERFACE_H
