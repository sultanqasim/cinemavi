#ifndef CMRAWIMAGE_H
#define CMRAWIMAGE_H

#include <cstdint>
#include <vector>
#include "../cmraw.h"

class CMRawImage
{
public:
    CMRawImage();
    void setImage(const void *raw, const CMCaptureInfo &cinfo);
    const void *getRaw() const;
    const CMCaptureInfo &getCaptureInfo() const;

private:
    std::vector<uint8_t> rawData;
    CMCaptureInfo cinfo;
};

#endif // CMRAWIMAGE_H
