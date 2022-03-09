#include "dng.h"
#include "debayer.h"
#include "cie_xyz.h"
#include "cm_calibrations.h"

#define TINY_DNG_WRITER_IMPLEMENTATION
#include "tiny_dng_writer.h"

int bayer_rg12p_to_dng(const void *raw, const CMRawHeader *cmrh, const char *dng_name)
{
    tinydngwriter::DNGImage dng_image;
    tinydngwriter::DNGWriter dng_writer(false); // little endian DNG

    // set some mandatory tags
    dng_image.SetDNGVersion(1, 5, 0, 0);
    dng_image.SetOrientation(tinydngwriter::ORIENTATION_TOPLEFT);
    dng_image.SetUniqueCameraModel(cmrh->camera_model);

    dng_image.SetBigEndian(false);
    dng_image.SetSubfileType(false, false, false);
    dng_image.SetImageWidth(cmrh->cinfo.width);
    dng_image.SetImageLength(cmrh->cinfo.height);
    dng_image.SetRowsPerStrip(cmrh->cinfo.height);
    dng_image.SetSamplesPerPixel(1);
    const uint16_t bpp[1] = {16};
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
    ColourMatrix cam_to_XYZ, XYZ_to_cam;
    colour_matmult33(&cam_to_XYZ, &CM_sRGB2XYZ, get_calibration(&cmrh->cinfo));
    colour_matinv33(&XYZ_to_cam, &cam_to_XYZ);
    dng_image.SetCalibrationIlluminant1(17); // StdA
    dng_image.SetCalibrationIlluminant2(21); // D65
    dng_image.SetColorMatrix1(3, XYZ_to_cam.m);
    dng_image.SetColorMatrix2(3, XYZ_to_cam.m);

    // White balance
    double r, b;
    if (cmrh->cinfo.white_x > 0 || cmrh->cinfo.white_y > 0)
        colour_illum_xy_to_rb_ratio(&cam_to_XYZ, cmrh->cinfo.white_x, cmrh->cinfo.white_y, &r, &b);
    else // default to D50 if no white point specified
        colour_temp_tint_to_rb_ratio(&cam_to_XYZ, 5000, 0, &r, &b);
    ColourPixel cam_neutral_RGB = {.p={1/r, 1, 1/b}};
    dng_image.SetAsShotNeutral(3, cam_neutral_RGB.p);

    std::vector<uint16_t> unpacked;
    unpacked.resize(cmrh->cinfo.width * cmrh->cinfo.height);
    unpack12_16(unpacked.data(), raw, cmrh->cinfo.width * cmrh->cinfo.height, true);
    dng_image.SetImageData((uint8_t *)unpacked.data(), unpacked.size() * sizeof(uint16_t));
    dng_writer.AddImage(&dng_image);

    std::string err;
    dng_writer.WriteToFile(dng_name, &err);
    if (!err.empty()) return -1;

    return 0;
}

int rgb8_to_tiff(const uint8_t *img, uint16_t width, uint16_t height, const char *tiff_name)
{
    tinydngwriter::DNGImage dng_image;
    tinydngwriter::DNGWriter dng_writer(false); // little endian DNG

    dng_image.SetBigEndian(false);
    dng_image.SetSubfileType(false, false, false);
    dng_image.SetImageWidth(width);
    dng_image.SetImageLength(height);
    dng_image.SetRowsPerStrip(height);

    dng_image.SetSamplesPerPixel(3);
    const uint16_t bpp[3] = {8, 8, 8};
    dng_image.SetBitsPerSample(3, bpp);
    const uint16_t sf[1] = {tinydngwriter::SAMPLEFORMAT_UINT};
    dng_image.SetSampleFormat(1, sf);
    dng_image.SetCompression(tinydngwriter::COMPRESSION_NONE);
    dng_image.SetPlanarConfig(tinydngwriter::PLANARCONFIG_CONTIG);

    dng_image.SetXResolution(1.0);
    dng_image.SetYResolution(1.0);
    dng_image.SetResolutionUnit(tinydngwriter::RESUNIT_NONE);

    dng_image.SetPhotometric(tinydngwriter::PHOTOMETRIC_RGB);

    dng_image.SetImageData(img, width * height * 3);
    dng_writer.AddImage(&dng_image);

    std::string err;
    dng_writer.WriteToFile(tiff_name, &err);
    if (!err.empty()) return -1;

    return 0;
}
