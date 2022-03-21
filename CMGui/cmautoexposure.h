#ifndef CMAUTOEXPOSURE_H
#define CMAUTOEXPOSURE_H

#include <QObject>
#include <QThread>
#include <QSemaphore>
#include "cmrawimage.h"
#include "../pipeline.h"

typedef enum {
    CMEXP_AUTO,
    CMEXP_SHUTTER_PRIORITY,
    CMEXP_GAIN_PRIORITY,
    CMEXP_MANUAL
} CMExposureMode;

class CMAutoExposure : public QObject
{
    Q_OBJECT
public:
    explicit CMAutoExposure();
    ~CMAutoExposure();
    void setParams(const ImagePipelineParams &params);

public slots:
    void setImage(const CMRawImage &img);

signals:
    void exposureChangeCalculated(double changeFactor);

private slots:
    void workLoop();

private:
    QThread workThread;
    QSemaphore workSem;
    CMRawImage rawImg;
    ImagePipelineParams plParams;
    volatile bool calculating = false;
    volatile bool done = false;
    unsigned delayCounter = 0;

    const double delayFrames = 2;
    const double filterFactor = 0.5;
};

#endif // CMAUTOEXPOSURE_H
