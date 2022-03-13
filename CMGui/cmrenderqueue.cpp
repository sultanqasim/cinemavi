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

    saveWorker.moveToThread(&saveThread);
    connect(&saveThread, &QThread::started, &saveWorker, &CMSaveWorker::save);
    connect(&saveWorker, &CMSaveWorker::imageSaved, this, &CMRenderQueue::saveDone);
}

CMRenderQueue::~CMRenderQueue() {
    renderThread.quit();
    renderThread.wait();
    saveThread.quit();
    saveThread.wait();
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

void CMRenderQueue::setRawImage(const CMRawImage &img)
{
    this->setImage(img.getRaw(), &img.getCaptureInfo());
}

void  CMRenderQueue::setParams(const ImagePipelineParams &params)
{
    this->plParams = params;
    this->paramsSet = true;
    if (rendering)
        renderQueued = true;
    else
        this->startRender();
}

void CMRenderQueue::startRender()
{
    if (!imageSet || !paramsSet)
        return;
    rendering = true;

    // prepare and launch worker
    worker.setImage(this->currentRaw.data(), &this->currentCInfo);
    worker.setParams(this->plParams);
    renderThread.start();
}

void CMRenderQueue::renderDone(const QPixmap &pm)
{
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

bool CMRenderQueue::autoWhiteBalance(const CMAutoWhiteParams &params, double *temp_K, double *tint)
{
    if (!imageSet)
        return false;

    pipeline_auto_white_balance(this->currentRaw.data(), &this->currentCInfo,
            &params, temp_K, tint);
    return true;
}

bool CMRenderQueue::saveImage(const QString &fileName)
{
    if (!imageSet || !paramsSet || saving)
        return false;

    saving = true;

    if (imageQueued)
        saveWorker.setParams(fileName.toStdString(), this->nextRaw.data(), this->nextCInfo,
                             this->plParams);
    else
        saveWorker.setParams(fileName.toStdString(), this->currentRaw.data(), this->currentCInfo,
                             this->plParams);

    saveThread.start();
    return true;
}

void CMRenderQueue::saveDone()
{
    saveThread.quit();
    saveThread.wait();
    saving = false;
    emit imageSaved();
}
