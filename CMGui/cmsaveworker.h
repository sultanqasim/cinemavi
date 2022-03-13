#ifndef CMSAVEWORKER_H
#define CMSAVEWORKER_H

#include <QObject>
#include "cmrawimage.h"
#include "../pipeline.h"

class CMSaveWorker : public QObject
{
    Q_OBJECT
public:
    explicit CMSaveWorker(QObject *parent = nullptr);
    void setParams(const std::string &fileName, const CMRawImage &img,
                  const ImagePipelineParams &params);

public slots:
    void save();

signals:
    void imageSaved();

private:
    ImagePipelineParams plParams;
    CMRawImage imgRaw;
    std::string fileName;
    bool paramsSet = false;
};

#endif // CMSAVEWORKER_H
