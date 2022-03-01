#ifndef CMRENDERWORKER_H
#define CMRENDERWORKER_H

#include <QObject>
#include <QPixmap>
#include "../pipeline.h"

class CMRenderWorker : public QObject
{
    Q_OBJECT
public:
    explicit CMRenderWorker(QObject *parent = nullptr);
    void setImage(const void *raw, const CMCaptureInfo *cinfo);
    void setParams(const ImagePipelineParams &params, const ColourMatrix &calib);

public slots:
    void render(void);

signals:
    void imageRendered(const QPixmap &pm);
    void finished();

private:
    ImagePipelineParams plParams;
    ColourMatrix camCalib;
    const CMCaptureInfo *cinfo;
    const void *imgRaw;

    // this is a one shot single use class
    bool paramsSet;
    bool rendering;
};

#endif // CMRENDERWORKER_H
