#include "cmsaveworker.h"
#include <cstring>
#include <QImage>
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

static bool endsWith(std::string const &s, std::string const &suffix)
{
    if (s.length() >= suffix.length())
        return (0 == s.compare(s.length() - suffix.length(), suffix.length(), suffix));
    else
        return false;
}

void CMSaveWorker::save()
{
    assert(this->paramsSet);

    int status = -1;

    if (endsWith(this->fileName, ".cmr")) {
        status = cmraw_save(this->imgRaw.getRaw(), &this->imgRaw.getRawHeader(),
                this->fileName.c_str());
    } else if (endsWith(this->fileName, ".dng")) {
        status = bayer_rg12p_to_dng(this->imgRaw.getRaw(), &this->imgRaw.getRawHeader(),
                this->fileName.c_str());
    } else if (endsWith(this->fileName, ".tiff") || endsWith(this->fileName, ".tif")) {
        std::vector<uint8_t> imgRgb8;
        const CMCaptureInfo &cinfo = this->imgRaw.getCaptureInfo();
        imgRgb8.resize(cinfo.width * cinfo.height * 3);
        status = pipeline_process_image(this->imgRaw.getRaw(), imgRgb8.data(), &cinfo, &this->plParams);
        if (status == 0)
            status = rgb8_to_tiff(imgRgb8.data(), cinfo.width, cinfo.height, this->fileName.c_str());
    } else if (endsWith(this->fileName, ".jpg") || endsWith(this->fileName, ".jpeg")) {
        std::vector<uint8_t> imgRgb8;
        const CMCaptureInfo &cinfo = this->imgRaw.getCaptureInfo();
        imgRgb8.resize(cinfo.width * cinfo.height * 3);
        status = pipeline_process_image(this->imgRaw.getRaw(), imgRgb8.data(), &cinfo, &this->plParams);
        if (status == 0) {
            QImage img(imgRgb8.data(), cinfo.width, cinfo.height, cinfo.width*3, QImage::Format_RGB888);
            if (img.save(QString::fromStdString(this->fileName)) != true)
                status = -1;
        }
    }

    this->paramsSet = false;
    this->imgRaw = CMRawImage(); // free memory

    emit imageSaved(status == 0);
}
