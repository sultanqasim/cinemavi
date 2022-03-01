#include "cmrenderworker.h"
#include <QImage>
#include <cassert>

CMRenderWorker::CMRenderWorker(QObject *parent)
    : QObject{parent}
{

}

void CMRenderWorker::setImage(const void *raw, const CMCaptureInfo *cinfo) {
    this->imgRaw = raw;
    this->cinfo = cinfo;
}

void CMRenderWorker::setParams(const ImagePipelineParams &params, const ColourMatrix &calib) {
    this->camCalib = calib;
    this->plParams = params;
    this->paramsSet = true;
}

void CMRenderWorker::render() {
    assert(this->paramsSet);
    assert(this->imgRaw != NULL && this->cinfo != NULL);

    uint16_t width_out = this->cinfo->width / 2;
    uint16_t height_out = this->cinfo->height / 2;

    std::vector<uint8_t> imgRgb8;
    imgRgb8.resize(width_out * height_out * 3);
    pipeline_process_image_bin22(this->imgRaw, imgRgb8.data(), this->cinfo,
                                 &this->plParams, &this->camCalib);
    QImage img(imgRgb8.data(), width_out, height_out, width_out*3, QImage::Format_RGB888);
    QPixmap pixmap;
    pixmap.convertFromImage(img);

    emit imageRendered(pixmap);
}
