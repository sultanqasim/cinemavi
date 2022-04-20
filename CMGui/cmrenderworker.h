#ifndef CMRENDERWORKER_H
#define CMRENDERWORKER_H

#include <QObject>
#include <QImage>
#include "cmrawimage.h"
#include "../pipeline.h"

class CMRenderWorker : public QObject
{
    Q_OBJECT
public:
    explicit CMRenderWorker(QObject *parent = nullptr);
    void setImage(const CMRawImage *img);
    void setParams(const ImagePipelineParams &params);

public slots:
    void render();

signals:
    void imageRendered(const QImage &img);

private:
    ImagePipelineParams plParams;
    const CMRawImage *imgRaw = NULL;
    bool paramsSet = false;
};

#endif // CMRENDERWORKER_H
