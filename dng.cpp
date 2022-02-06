#include "dng.h"
#include "debayer.h"

#define TINY_DNG_WRITER_IMPLEMENTATION
#include "tiny_dng_writer.h"

int bayer_rg12p_to_dng(const void *raw, uint16_t width, uint16_t height,
        const char *dng_name, const char *camera_model)
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

    // Colour calibration (use Adobe RGB primaries)
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    ColourMatrix XYZtoRGB_D50 = {.m={
        1.9624274, -0.6105343, -0.3413404,
       -0.9787684,  1.9161415,  0.0334540,
        0.0286869, -0.1406752,  1.3487655
    }};
    ColourMatrix XYZtoRGB_D65 = {.m={
        2.0413690, -0.5649464, -0.3446944,
       -0.9692660,  1.8760108,  0.0415560,
        0.0134474, -0.1183897,  1.0154096
    }};
    dng_image.SetCalibrationIlluminant1(23); // D50
    dng_image.SetCalibrationIlluminant2(21); // D65
    dng_image.SetColorMatrix1(3, XYZtoRGB_D50.m);
    dng_image.SetColorMatrix2(3, XYZtoRGB_D65.m);
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
