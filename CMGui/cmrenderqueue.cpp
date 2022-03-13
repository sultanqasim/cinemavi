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

void CMRenderQueue::setImage(const CMRawImage &img)
{
    this->imageSet = true;
    if (rendering) {
        this->nextRaw = img;
        imageQueued = true;
        renderQueued = true;
    } else {
        this->currentRaw = img;
        this->startRender();
    }
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
    worker.setImage(&this->currentRaw);
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

    pipeline_auto_white_balance(this->currentRaw.getRaw(), &this->currentRaw.getCaptureInfo(),
            &params, temp_K, tint);
    return true;
}

bool CMRenderQueue::saveImage(const QString &fileName)
{
    if (!imageSet || !paramsSet || saving)
        return false;

    saving = true;

    if (imageQueued)
        saveWorker.setParams(fileName.toStdString(), this->nextRaw, this->plParams);
    else
        saveWorker.setParams(fileName.toStdString(), this->currentRaw, this->plParams);

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
