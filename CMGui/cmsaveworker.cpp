#include "cmsaveworker.h"
#include <cstring>
#include <QImage>
#include "../dng.h"
#include "../colour_xfrm.h"

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

    // make capture info match "as shot" white balance
    CMRawHeader cmrh = this->imgRaw.getRawHeader();
    double white_x, white_y;
    colour_temp_tint_to_xy(this->plParams.temp_K, this->plParams.tint, &white_x, &white_y);
    cmrh.cinfo.white_x = white_x;
    cmrh.cinfo.white_y = white_y;

    if (endsWith(this->fileName, ".cmr")) {
        status = cmraw_save(this->imgRaw.getRaw(), &cmrh, this->fileName.c_str());
    } else if (endsWith(this->fileName, ".dng")) {
        status = bayer_rg12p_to_dng(this->imgRaw.getRaw(), &cmrh, this->fileName.c_str());
    } else if (endsWith(this->fileName, ".tiff") || endsWith(this->fileName, ".tif")) {
        std::vector<uint8_t> imgRgb8;
        imgRgb8.resize(cmrh.cinfo.width * cmrh.cinfo.height * 3);
        status = pipeline_process_image(this->imgRaw.getRaw(), imgRgb8.data(), &cmrh.cinfo, &this->plParams);
        if (status == 0)
            status = rgb8_to_tiff(imgRgb8.data(), cmrh.cinfo.width, cmrh.cinfo.height, this->fileName.c_str());
    } else if (endsWith(this->fileName, ".jpg") || endsWith(this->fileName, ".jpeg")) {
        std::vector<uint8_t> imgRgb8;
        imgRgb8.resize(cmrh.cinfo.width * cmrh.cinfo.height * 3);
        status = pipeline_process_image(this->imgRaw.getRaw(), imgRgb8.data(), &cmrh.cinfo, &this->plParams);
        if (status == 0) {
            QImage img(imgRgb8.data(), cmrh.cinfo.width, cmrh.cinfo.height, cmrh.cinfo.width*3, QImage::Format_RGB888);
            if (img.save(QString::fromStdString(this->fileName)) != true)
                status = -1;
        }
    }

    this->paramsSet = false;
    this->imgRaw = CMRawImage(); // free memory

    emit imageSaved(status == 0);
}
