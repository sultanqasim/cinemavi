#include "dng.h"
#include "debayer.h"

#define TINY_DNG_WRITER_IMPLEMENTATION
#include "tiny_dng_writer.h"

int arv_buffer_to_dng(ArvBuffer *buffer, const char *dng_name, const char *camera_model)
{
    int width = arv_buffer_get_image_width(buffer);
    int height = arv_buffer_get_image_height(buffer);
    tinydngwriter::DNGImage dng_image;
    tinydngwriter::DNGWriter dng_writer(false); // little endian DNG

    if (arv_buffer_get_payload_type(buffer) != ARV_BUFFER_PAYLOAD_TYPE_IMAGE)
        return -2;

    // set some mandatory tags
    dng_image.SetDNGVersion(1, 5, 0, 0);
    dng_image.SetOrientation(tinydngwriter::ORIENTATION_TOPLEFT);
    dng_image.SetUniqueCameraModel(camera_model);

    dng_image.SetBigEndian(false);
    dng_image.SetSubfileType(false, false, false);
    dng_image.SetImageWidth(width);
    dng_image.SetImageLength(height);
    dng_image.SetRowsPerStrip(height);
    dng_image.SetSamplesPerPixel(1);
    const uint16_t bpp[1] = {16}; //{12};
    dng_image.SetBitsPerSample(1, bpp);
    const uint16_t sf[1] = {tinydngwriter::SAMPLEFORMAT_UINT};
    dng_image.SetSampleFormat(1, sf);
    dng_image.SetCompression(tinydngwriter::COMPRESSION_NONE);
    dng_image.SetPlanarConfig(tinydngwriter::PLANARCONFIG_CONTIG);

    dng_image.SetXResolution(1.0);
    dng_image.SetYResolution(1.0);
    dng_image.SetResolutionUnit(tinydngwriter::RESUNIT_NONE);

    // Bayer pattern config
    dng_image.SetPhotometric(tinydngwriter::PHOTOMETRIC_CFA);
    dng_image.SetCFARepeatPatternDim(2, 2);
    const uint8_t cpat[4] = {0, 1, 1, 2};
    dng_image.SetCFAPattern(4, cpat);

    // Colour calibration
    ColourMatrix XYZtoRGB;
    colour_matrix(&XYZtoRGB, 0, 0.0, -0.45, 1.7);
    dng_image.SetCalibrationIlluminant1(17); // StdA
    dng_image.SetCalibrationIlluminant2(21); // D65
    dng_image.SetColorMatrix1(3, XYZtoRGB.m);
    dng_image.SetColorMatrix2(3, XYZtoRGB.m);
    const double idmat[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    dng_image.SetCameraCalibration1(3, idmat);
    dng_image.SetCameraCalibration2(3, idmat);
    dng_image.SetAsShotWhiteXY(1.0 / 3, 1.0 / 3);

    size_t imsz;
    const uint8_t *imbuf = (const uint8_t *) arv_buffer_get_data(buffer, &imsz);
    ArvPixelFormat pfmt = arv_buffer_get_image_pixel_format(buffer);
    std::vector<uint16_t> unpacked;
    if (ARV_PIXEL_FORMAT_BIT_PER_PIXEL(pfmt) == 12) {
        unpacked.resize(width * height);
        unpack12_16(unpacked.data(), imbuf, width*height, true);
        dng_image.SetImageData((uint8_t *)unpacked.data(), unpacked.size() * sizeof(uint16_t));
    } else if (ARV_PIXEL_FORMAT_BIT_PER_PIXEL(pfmt) == 16) {
        dng_image.SetImageData(imbuf, imsz);
    } else {
        return -3;
    }
    dng_writer.AddImage(&dng_image);

    std::string err;
    dng_writer.WriteToFile(dng_name, &err);
    if (!err.empty()) return -1;

    return 0;
}
