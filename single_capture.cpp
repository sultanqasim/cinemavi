#include <cstdlib>
#include <cstdio>
#include <cstdint>

#define TINY_DNG_WRITER_IMPLEMENTATION
#include "tiny_dng_writer.h"

extern "C" {
#include <arv.h>
}

static void cinemavi_camera_configure(ArvCamera *camera, double shutter_us, double gain_db, GError **error)
{
    int wmin, wmax, hmin, hmax;

    // Set image size to use full sensor
    if (!(*error)) arv_camera_get_width_bounds(camera, &wmin, &wmax, error);
    if (!(*error)) arv_camera_get_height_bounds(camera, &hmin, &hmax, error);
    if (!(*error)) arv_camera_set_region(camera, 0, 0, wmax, hmax, error);
    //if (!(*error)) arv_camera_set_pixel_format(camera, ARV_PIXEL_FORMAT_BAYER_RG_12P, error);
    if (!(*error)) arv_camera_set_pixel_format(camera, ARV_PIXEL_FORMAT_BAYER_RG_16, error);
    if (!(*error)) arv_camera_set_binning(camera, 1, 1, error);

    // Exposure
    if (!(*error)) arv_camera_set_exposure_time_auto(camera, ARV_AUTO_OFF, error);
    if (!(*error)) arv_camera_set_exposure_time(camera, shutter_us, error);
    if (!(*error)) arv_camera_set_gain_auto(camera, ARV_AUTO_OFF, error);
    if (!(*error)) arv_camera_set_gain(camera, gain_db, error);

    // White balance (make it neutral)
    if (!(*error)) arv_camera_set_string(camera, "BalanceWhiteAuto", "Off", error);
    if (!(*error)) arv_camera_set_float(camera, "BalanceRatio", 1.0, error);
}

static int cinemavi_buffer_to_dng(ArvBuffer *buffer, const char *dng_name)
{
    int width = arv_buffer_get_image_width(buffer);
    int height = arv_buffer_get_image_height(buffer);
    tinydngwriter::DNGImage dng_image;
    tinydngwriter::DNGWriter dng_writer(false); // little endian DNG

    // set some mandatory tags
    dng_image.SetDNGVersion(1, 5, 0, 0);
    dng_image.SetOrientation(tinydngwriter::ORIENTATION_TOPLEFT);
    dng_image.SetUniqueCameraModel("Cinemavi");

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

    size_t imsz;
    const uint8_t *imbuf = (const uint8_t *) arv_buffer_get_data(buffer, &imsz);
    dng_image.SetImageData(imbuf, imsz);
    dng_writer.AddImage(&dng_image);

    std::string err;
    dng_writer.WriteToFile(dng_name, &err);
    if (!err.empty()) {
        printf("%s", err.c_str());
        return -1;
    }

    return 0;
}

// Connect to the first available camera, then acquire a single buffer.
int main (int argc, char **argv)
{
    ArvCamera *camera;
    ArvBuffer *buffer;
    GError *error = NULL;

    // Connect to the first available camera
    camera = arv_camera_new(NULL, &error);

    if (ARV_IS_CAMERA(camera)) {
        printf("Found camera '%s'\n", arv_camera_get_model_name(camera, NULL));

        // Set configuration
        cinemavi_camera_configure(camera, 50000, 5, &error);
        if (error) goto err;

        // Acquire a single buffer
        buffer = arv_camera_acquisition(camera, 0, &error);

        if (ARV_IS_BUFFER(buffer)) {
            // Display some informations about the retrieved buffer
            printf("Acquired %d×%d buffer\n",
                    arv_buffer_get_image_width(buffer),
                    arv_buffer_get_image_height(buffer));

            if (argc > 1) {
                printf("Writing buffer to DNG: %s\n", argv[1]);
                int dng_stat = cinemavi_buffer_to_dng(buffer, argv[1]);
                if (dng_stat != 0) printf("Error %d writing DNG.\n", dng_stat);
                else printf("DNG written.\n");
            }

            // Destroy the buffer
            g_clear_object(&buffer);
        }

err:
        /* Destroy the camera instance */
        g_clear_object (&camera);
    }

    if (error != NULL) {
        /* En error happened, display the correspdonding message */
        printf("Error: %s\n", error->message);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
