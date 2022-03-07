#include "cmsaveworker.h"
#include <cstring>
#include "../dng.h"

CMSaveWorker::CMSaveWorker(QObject *parent)
    : QObject{parent}
{

}

void CMSaveWorker::setParams(const std::string &fileName, const void *raw, const CMCaptureInfo &cinfo,
                             const ImagePipelineParams &params, const ColourMatrix &calib)
{
    this->fileName = fileName;
    this->cinfo = cinfo;
    this->camCalib = calib;
    this->plParams = params;
    size_t rawSize = (cinfo.width * 3 / 2) * cinfo.height;
    this->imgRaw.resize(rawSize);
    memcpy(this->imgRaw.data(), raw, rawSize);
    this->paramsSet = true;
}

void CMSaveWorker::save()
{
    assert(this->paramsSet);

    std::vector<uint8_t> imgRgb8;
    imgRgb8.resize(this->cinfo.width * this->cinfo.height * 3);
    pipeline_process_image(this->imgRaw.data(), imgRgb8.data(), &this->cinfo,
                           &this->plParams, &this->camCalib);
    rgb8_to_tiff(imgRgb8.data(), this->cinfo.width, this->cinfo.height, this->fileName.c_str());

    this->paramsSet = false;
    this->imgRaw.resize(0); // free memory

    emit imageSaved();
}
