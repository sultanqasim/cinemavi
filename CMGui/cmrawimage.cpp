#include "cmrawimage.h"
#include <cstring>

CMRawImage::CMRawImage()
{
    memset(&this->cmrh, 0, sizeof(this->cmrh));
}

void CMRawImage::setImage(const void *raw, const CMRawHeader &cmrh)
{
    size_t imgSz;
    if (cmrh.cinfo.pixel_fmt == CM_PIXEL_FMT_BAYER_RG12P ||
            cmrh.cinfo.pixel_fmt == CM_PIXEL_FMT_MONO12P)
        imgSz = (cmrh.cinfo.width * 3 / 2) * cmrh.cinfo.height;
    else if (cmrh.cinfo.pixel_fmt == CM_PIXEL_FMT_BAYER_RG12 ||
             cmrh.cinfo.pixel_fmt == CM_PIXEL_FMT_BAYER_RG16 ||
             cmrh.cinfo.pixel_fmt == CM_PIXEL_FMT_MONO12 ||
             cmrh.cinfo.pixel_fmt == CM_PIXEL_FMT_MONO16)
        imgSz = cmrh.cinfo.width * cmrh.cinfo.height * 2;
    else
        imgSz = 0; // unsupported for now
    this->rawData.resize(imgSz);
    memcpy(this->rawData.data(), raw, imgSz);
    this->cmrh = cmrh;
}

const void * CMRawImage::getRaw() const
{
    return this->rawData.data();
}

const CMCaptureInfo & CMRawImage::getCaptureInfo() const
{
    return this->cmrh.cinfo;
}

const CMRawHeader & CMRawImage::getRawHeader() const
{
    return this->cmrh;
}

bool CMRawImage::isEmpty() const
{
    uint32_t imSz = this->cmrh.cinfo.width * this->cmrh.cinfo.height;
    return imSz == 0;
}
