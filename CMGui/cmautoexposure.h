#ifndef CMAUTOEXPOSURE_H
#define CMAUTOEXPOSURE_H

#include <QObject>
#include <QThread>
#include <QSemaphore>
#include "cmrawimage.h"
#include "../pipeline.h"

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

    const double filterFactor = 0.5;
};

#endif // CMAUTOEXPOSURE_H
