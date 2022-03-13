#include "cmrawimage.h"
#include <cstring>

CMRawImage::CMRawImage()
{
    memset(&this->cinfo, 0, sizeof(this->cinfo));
}

void CMRawImage::setImage(const void *raw, const CMCaptureInfo &cinfo)
{
    size_t imgSz;
    if (cinfo.pixel_fmt == CM_PIXEL_FMT_BAYER_RG12P || cinfo.pixel_fmt == CM_PIXEL_FMT_MONO12P)
        imgSz = (cinfo.width * 3 / 2) * cinfo.height;
    else if (cinfo.pixel_fmt == CM_PIXEL_FMT_BAYER_RG12 ||
             cinfo.pixel_fmt == CM_PIXEL_FMT_BAYER_RG16 ||
             cinfo.pixel_fmt == CM_PIXEL_FMT_MONO12 ||
             cinfo.pixel_fmt == CM_PIXEL_FMT_MONO16)
        imgSz = cinfo.width * cinfo.height * 2;
    else
        imgSz = 0; // unsupported for now
    this->rawData.resize(imgSz);
    memcpy(this->rawData.data(), raw, imgSz);
    this->cinfo = cinfo;
}

const void * CMRawImage::getRaw() const
{
    return this->rawData.data();
}

const CMCaptureInfo & CMRawImage::getCaptureInfo() const
{
    return this->cinfo;
}

bool CMRawImage::isEmpty() const
{
    uint32_t imSz = this->cinfo.width * this->cinfo.height;
    return imSz == 0;
}
