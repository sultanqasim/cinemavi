#include "cmrenderqueue.h"
#include "cmrenderworker.h"
#include <cstring>
#include <QThread>

CMRenderQueue::CMRenderQueue(QObject *parent)
    : QObject{parent}
{
    // set up worker in its thread
    worker.moveToThread(&renderThread);
    connect(&renderThread, &QThread::started, &worker, &CMRenderWorker::render);
    connect(&worker, &CMRenderWorker::imageRendered, this, &CMRenderQueue::renderDone);
}

CMRenderQueue::~CMRenderQueue() {
    renderThread.quit();
    renderThread.wait();
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

    // prepare and launch worker
    worker.setImage(this->currentRaw.data(), &this->currentCInfo);
    worker.setParams(this->plParams, this->camCalib);
    renderThread.start();
}

void CMRenderQueue::renderDone(const QPixmap &pm) {
    emit imageRendered(pm);
    renderThread.quit();
    renderThread.wait();

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
