#include "cmsaveworker.h"
#include <cstring>
#include "../dng.h"

CMSaveWorker::CMSaveWorker(QObject *parent)
    : QObject{parent}
{

}

void CMSaveWorker::setParams(const std::string &fileName, const CMRawImage &img,
                             const ImagePipelineParams &params)
{
    this->fileName = fileName;
    this->imgRaw = img;
    this->plParams = params;
    this->paramsSet = true;
}

void CMSaveWorker::save()
{
    assert(this->paramsSet);

    std::vector<uint8_t> imgRgb8;
    const CMCaptureInfo &cinfo = this->imgRaw.getCaptureInfo();
    imgRgb8.resize(cinfo.width * cinfo.height * 3);
    pipeline_process_image(this->imgRaw.getRaw(), imgRgb8.data(), &cinfo, &this->plParams);
    rgb8_to_tiff(imgRgb8.data(), cinfo.width, cinfo.height, this->fileName.c_str());

    this->paramsSet = false;
    this->imgRaw = CMRawImage(); // free memory

    emit imageSaved();
}
