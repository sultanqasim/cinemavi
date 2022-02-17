#include "dng.h"
#include "debayer.h"

#define TINY_DNG_WRITER_IMPLEMENTATION
#include "tiny_dng_writer.h"

int bayer_rg12p_to_dng(const void *raw, uint16_t width, uint16_t height,
        const char *dng_name, const char *camera_model, const ColourMatrix *calib)
{
    tinydngwriter::DNGImage dng_image;
    tinydngwriter::DNGWriter dng_writer(false); // little endian DNG

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
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    const ColourMatrix sRGB_to_XYZ = {.m={
        0.4124564, 0.3575761, 0.1804375,
        0.2126729, 0.7151522, 0.0721750,
        0.0193339, 0.1191920, 0.9503041
     }};
    ColourMatrix cam_to_XYZ_D65, XYZ_to_cam_D65;
    // multiplication below it backwards, but doing it the right way gives wrong colours
    // I'm not entirely sure what's happening in DNG processors
    colour_matmult33(&cam_to_XYZ_D65, calib, &sRGB_to_XYZ);
    colour_matinv33(&XYZ_to_cam_D65, &cam_to_XYZ_D65);

    // https://onlinelibrary.wiley.com/doi/pdf/10.1002/9781119021780.app3
    const ColourMatrix Bradford_D65_to_StdA = {.m={
         1.2191, -0.0489,  0.4138,
        -0.2952,  0.9106, -0.0676,
         0.0202, -0.0251,  0.3168
    }};
    ColourMatrix cam_to_XYZ_StdA, XYZ_to_cam_StdA;
    // again the matrix multiplication needs to be backwards to work properly for some reason
    colour_matmult33(&cam_to_XYZ_StdA, &cam_to_XYZ_D65, &Bradford_D65_to_StdA);
    colour_matinv33(&XYZ_to_cam_StdA, &cam_to_XYZ_StdA);

    dng_image.SetCalibrationIlluminant1(17); // StdA
    dng_image.SetCalibrationIlluminant2(21); // D65
    dng_image.SetColorMatrix1(3, XYZ_to_cam_StdA.m);
    dng_image.SetColorMatrix2(3, XYZ_to_cam_D65.m);
    dng_image.SetAsShotWhiteXY(1.0 / 3, 1.0 / 3);

    std::vector<uint16_t> unpacked;
    unpacked.resize(width * height);
    unpack12_16(unpacked.data(), raw, width * height, true);
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
