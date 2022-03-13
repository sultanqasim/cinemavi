#include "cmrenderworker.h"
#include <QImage>
#include <cassert>

CMRenderWorker::CMRenderWorker(QObject *parent)
    : QObject{parent}
{

}

void CMRenderWorker::setImage(const CMRawImage *img) {
    this->imgRaw = img;
}

void CMRenderWorker::setParams(const ImagePipelineParams &params) {
    this->plParams = params;
    this->paramsSet = true;
}

void CMRenderWorker::render() {
    assert(this->paramsSet);
    assert(this->imgRaw != NULL);

    if (this->imgRaw->isEmpty()) {
        emit imageRendered(QPixmap());
        return;
    }

    uint16_t width_out = this->imgRaw->getCaptureInfo().width / 2;
    uint16_t height_out = this->imgRaw->getCaptureInfo().height / 2;

    std::vector<uint8_t> imgRgb8;
    imgRgb8.resize(width_out * height_out * 3);
    pipeline_process_image_bin22(this->imgRaw->getRaw(), imgRgb8.data(),
                                 &this->imgRaw->getCaptureInfo(), &this->plParams);
    QImage img(imgRgb8.data(), width_out, height_out, width_out*3, QImage::Format_RGB888);
    QPixmap pixmap;
    pixmap.convertFromImage(img);

    emit imageRendered(pixmap);
}
