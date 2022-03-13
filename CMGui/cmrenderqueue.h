#ifndef CMRENDERQUEUE_H
#define CMRENDERQUEUE_H

#include <QObject>
#include <QPixmap>
#include <QThread>
#include <QString>
#include "cmrenderworker.h"
#include "cmsaveworker.h"
#include "cmrawimage.h"

class CMRenderQueue : public QObject
{
    Q_OBJECT
public:
    explicit CMRenderQueue(QObject *parent = nullptr);
    ~CMRenderQueue();

    // always call from a single thread
    void setParams(const ImagePipelineParams &params);
    bool autoWhiteBalance(const CMAutoWhiteParams &params, double *temp_K, double *tint);
    bool saveImage(const QString &fileName);
    void setImageLater(const CMRawImage &img);

public slots:
    void setImage(const CMRawImage &img);
    void renderDone(const QPixmap &pm);
    void saveDone();

signals:
    void imageRendered(const QPixmap &pm);
    void imageSaved();

private:
    QThread renderThread;
    CMRenderWorker worker;
    QThread saveThread;
    CMSaveWorker saveWorker;

    bool paramsSet = false;

    bool saving = false;        // indicates a save is in progress
    bool rendering = false;     // indicates a render is in progress
    bool imageQueued = false;   // indicates if next image needs to be made current
    bool renderQueued = false;  // indicates if a new render should be done after last finishes
    CMRawImage currentRaw;
    CMRawImage nextRaw;
    ImagePipelineParams plParams;

    void startRender();
};

#endif // CMRENDERQUEUE_H
