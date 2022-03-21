#ifndef CMRAWIMAGE_H
#define CMRAWIMAGE_H

#include <cstdint>
#include <vector>
#include <string>
#include "../cmraw.h"

class CMRawImage
{
public:
    CMRawImage();
    void setImage(const void *raw, const CMRawHeader &cmrh);
    const void *getRaw() const;
    const CMCaptureInfo &getCaptureInfo() const;
    const CMRawHeader &getRawHeader() const;
    bool isEmpty() const;

private:
    std::vector<uint8_t> rawData;
    CMRawHeader cmrh;
    std::string cameraMake;
    std::string cameraModel;
};

#endif // CMRAWIMAGE_H
