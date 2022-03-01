#include "cmrenderqueue.h"
#include "cmrenderworker.h"
#include <cstring>
#include <QThread>

CMRenderQueue::CMRenderQueue(QObject *parent)
    : QObject{parent}
{

}

void CMRenderQueue::setImage(const void *raw, const CMCaptureInfo *cinfo)
{
    // packed 12 bit bayer data
    uint32_t rawSize = (cinfo->width/2) * cinfo->height * 3;
    this->imageSet = true;
    if (rendering) {
        this->nextCInfo = *cinfo;
        this->nextRaw.resize(rawSize);
        memcpy(this->nextRaw.data(), raw, rawSize);
        imageQueued = true;
        renderQueued = true;
    } else {
        this->currentCInfo = *cinfo;
        this->currentRaw.resize(rawSize);
        memcpy(this->currentRaw.data(), raw, rawSize);
        this->startRender();
    }
}

void CMRenderQueue::setCalib(const ColourMatrix &calib) {
    this->camCalib = calib;
    this->calibSet = true;
    if (rendering)
        renderQueued = true;
    else
        this->startRender();
}

void  CMRenderQueue::setParams(const ImagePipelineParams &params) {
    this->plParams = params;
    this->paramsSet = true;
    if (rendering)
        renderQueued = true;
    else
        this->startRender();
}

void CMRenderQueue::startRender() {
    if (!imageSet || !calibSet || !paramsSet)
        return;
    rendering = true;

    // create worker thread
    QThread *thread = new QThread;
    CMRenderWorker *worker = new CMRenderWorker;
    worker->setImage(this->currentRaw.data(), &this->currentCInfo);
    worker->setParams(this->plParams, this->camCalib);
    worker->moveToThread(thread);
    connect(thread, SIGNAL(started()), worker, SLOT(render()));
    connect(worker, &CMRenderWorker::imageRendered, this, &CMRenderQueue::renderDone);
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}

void CMRenderQueue::renderDone(const QPixmap &pm) {
    emit imageRendered(pm);

    if (imageQueued) {
        currentRaw = nextRaw;
        currentCInfo = nextCInfo;
        imageQueued = false;
    }

    if (renderQueued) {
        startRender();
        renderQueued = false;
    } else {
        rendering = false;
    }
}
