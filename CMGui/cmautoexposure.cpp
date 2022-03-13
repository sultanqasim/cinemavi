#include "cmautoexposure.h"

CMAutoExposure::CMAutoExposure()
{
    // run workLoop in its own thread
    this->moveToThread(&workThread);
    connect(&workThread, &QThread::started, this, &CMAutoExposure::workLoop);
    workThread.start();
}

CMAutoExposure::~CMAutoExposure()
{
    this->done = true;
    this->workSem.release();
    this->workThread.quit();
    this->workThread.wait();
}

void CMAutoExposure::setParams(const ImagePipelineParams &params)
{
    this->plParams = params;
}

void CMAutoExposure::setImage(const CMRawImage &img)
{
    if (!this->calculating) {
        this->calculating = true;
        this->rawImg = img;
        this->workSem.release();
    }
}

void CMAutoExposure::workLoop()
{
    while (!this->done) {
        this->workSem.acquire();
        if (this->done) break;
        double changeFactor;
        int status = pipeline_auto_exposure(this->rawImg.getRaw(),
                &this->rawImg.getCaptureInfo(), &this->plParams, &changeFactor);
        this->calculating = false;
        if (!status) {
            changeFactor = (1 - this->filterFactor)*changeFactor + this->filterFactor;
            emit exposureChangeCalculated(changeFactor);
        }
    }
}
