#ifndef CMRENDERQUEUE_H
#define CMRENDERQUEUE_H

#include <QObject>
#include <QPixmap>
#include <QThread>
#include "cmrenderworker.h"

class CMRenderQueue : public QObject
{
    Q_OBJECT
public:
    explicit CMRenderQueue(QObject *parent = nullptr);
    ~CMRenderQueue();

    // always call from a single thread
    void setImage(const void *raw, const CMCaptureInfo *cinfo);
    void setCalib(const ColourMatrix &calib);
    void setParams(const ImagePipelineParams &params);
    bool autoWhiteBalance(double *temp_K, double *tint);

public slots:
    void renderDone(const QPixmap &pm);

signals:
    void imageRendered(const QPixmap &pm);

private:
    QThread renderThread;
    CMRenderWorker worker;

    bool imageSet;
    bool calibSet;
    bool paramsSet;

    bool rendering;     // indicates a render is in progress
    bool imageQueued;   // indicates if next image needs to be made current
    bool renderQueued;  // indicates if a new render should be done after last finishes
    std::vector<uint8_t> currentRaw;
    CMCaptureInfo currentCInfo;
    std::vector<uint8_t> nextRaw;
    CMCaptureInfo nextCInfo;
    ColourMatrix camCalib;
    ImagePipelineParams plParams;

    void startRender();
};

#endif // CMRENDERQUEUE_H
