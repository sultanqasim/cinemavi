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
    void setParams(const ImagePipelineParams &params);

public slots:
    void render();

signals:
    void imageRendered(const QPixmap &pm);

private:
    ImagePipelineParams plParams;
    const CMCaptureInfo *cinfo;
    const void *imgRaw = NULL;
    bool paramsSet = false;
};

#endif // CMRENDERWORKER_H
