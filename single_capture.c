#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <arv.h>

#include "dng.h"
#include "cmraw.h"
#include "pipeline.h"

static void cinemavi_camera_configure(ArvCamera *camera, double shutter_us, double gain_db, GError **error)
{
    int wmin, wmax, hmin, hmax;

    // Set image size to use full sensor
    if (!(*error)) arv_camera_get_width_bounds(camera, &wmin, &wmax, error);
    if (!(*error)) arv_camera_get_height_bounds(camera, &hmin, &hmax, error);
    if (!(*error)) arv_camera_set_region(camera, 0, 0, wmax, hmax, error);
    if (!(*error)) arv_camera_set_pixel_format(camera, ARV_PIXEL_FORMAT_BAYER_RG_12P, error);
    if (!(*error)) arv_camera_set_binning(camera, 1, 1, error);

    // Exposure
    if (!(*error)) arv_camera_set_exposure_time_auto(camera, ARV_AUTO_OFF, error);
    if (!(*error)) arv_camera_set_exposure_time(camera, shutter_us, error);
    if (!(*error)) arv_camera_set_gain_auto(camera, ARV_AUTO_OFF, error);
    if (!(*error)) arv_camera_set_gain(camera, gain_db, error);

    // White balance (make it neutral)
    if (!(*error)) arv_camera_set_string(camera, "BalanceWhiteAuto", "Continuous", error);
    if (!(*error)) arv_camera_set_string(camera, "BalanceWhiteAuto", "Off", error);
    if (!(*error)) arv_camera_set_string(camera, "BalanceRatioSelector", "Red", error);
    if (!(*error)) arv_camera_set_float(camera, "BalanceRatio", 1.0, error);
    if (!(*error)) arv_camera_set_string(camera, "BalanceRatioSelector", "Blue", error);
    if (!(*error)) arv_camera_set_float(camera, "BalanceRatio", 1.0, error);
}

static const void * cinemavi_prepare_header(ArvBuffer *buffer, CMRawHeader *cmrh,
        const char *cam_make, const char *cam_model, float shutter, float gain)
{
    size_t imsz;
    const void *imbuf = (const void *)arv_buffer_get_data(buffer, &imsz);

    cm_raw_header_init(cmrh);

    if (arv_buffer_get_payload_type(buffer) != ARV_BUFFER_PAYLOAD_TYPE_IMAGE)
        return NULL;

    cmrh->cinfo.width = arv_buffer_get_image_width(buffer);
    cmrh->cinfo.height = arv_buffer_get_image_height(buffer);

    ArvPixelFormat pfmt = arv_buffer_get_image_pixel_format(buffer);
    if (pfmt == ARV_PIXEL_FORMAT_BAYER_RG_12P)
        cmrh->cinfo.pixel_fmt = CM_PIXEL_FMT_BAYER_RG12P;
    else if (pfmt == ARV_PIXEL_FORMAT_BAYER_RG_12)
        cmrh->cinfo.pixel_fmt = CM_PIXEL_FMT_BAYER_RG12;
    else
        return NULL;

    snprintf(cmrh->camera_make, sizeof(cmrh->camera_make), "%s", cam_make);
    snprintf(cmrh->camera_model, sizeof(cmrh->camera_model), "%s", cam_model);
    snprintf(cmrh->capture_software, sizeof(cmrh->capture_software), "Cinemavi CLI");

    cmrh->cinfo.shutter_us = shutter;
    cmrh->cinfo.gain_dB = gain;

    // hard code these for now
    cmrh->cinfo.focal_len_mm = 8.0;
    cmrh->cinfo.pixel_pitch_um = 3.45;

    return imbuf;
}

static void cinemavi_generate_dng(const void *raw, const CMRawHeader *cmrh,
        const char *fname)
{
    int dng_stat = bayer_rg12p_to_dng(raw, cmrh->cinfo.width, cmrh->cinfo.height, fname,
            cmrh->camera_model);
    if (dng_stat != 0) printf("Error %d writing DNG.\n", dng_stat);
    else printf("DNG written to: %s\n", fname);
}

static void cinemavi_generate_tiff(const void *raw, const CMRawHeader *cmrh,
        const char *fname)
{
    uint8_t *rgb8 = (uint8_t *)malloc(cmrh->cinfo.width * cmrh->cinfo.height * 3);
    if (rgb8 != NULL) {
        ImagePipelineParams params = {
            .exposure = 1.0,
            .warmth = -0.2,
            .tint = 0.3,
            .hue = 0.0,
            .sat = 1.0,
            .nr_lum = 150.0,
            .nr_chrom = 600.0,
            .gamma = 0.0,
            .shadow = 0.3,
            .lut_mode = CMLUT_CUBIC
        };
        printf("Processing image...\n");
        pipeline_process_image(raw, rgb8, &cmrh->cinfo, &params);
        printf("Image processed.\n");

        int tiff_stat = rgb8_to_tiff(rgb8, cmrh->cinfo.width, cmrh->cinfo.height, fname);
        if (tiff_stat != 0) printf("Error %d writing TIFF.\n", tiff_stat);
        else printf("TIFF written to: %s\n", fname);
    }
    free(rgb8);
}

static void cinemavi_generate_cmr(const void *raw, const CMRawHeader *cmrh,
        const char *fname)
{
    int cmr_stat = cmraw_save(raw, cmrh, fname);
    if (cmr_stat != 0)
        printf("Error %d writing CMR.\n", cmr_stat);
    else
        printf("CMR written to: %s\n", fname);
}

static bool endswith(const char *s, const char *suffix)
{
    size_t s_len = strlen(s);
    size_t e_len = strlen(suffix);

    if (e_len > s_len) return false;

    return memcmp(s + s_len - e_len, suffix, e_len) ? false : true;
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
        const char *camera_make = arv_camera_get_vendor_name(camera, NULL);
        const char *camera_model = arv_camera_get_model_name(camera, NULL);
        printf("Found camera '%s %s'\n", camera_make, camera_model);

        // Set configuration
        const double shutter_us = 30000;
        const double gain_dB = 5;
        cinemavi_camera_configure(camera, shutter_us, gain_dB, &error);
        if (error) goto err;

        // Acquire a single buffer
        buffer = arv_camera_acquisition(camera, 0, &error);

        if (ARV_IS_BUFFER(buffer)) {
            // Display some informations about the retrieved buffer
            unsigned int width = arv_buffer_get_image_width(buffer);
            unsigned int height = arv_buffer_get_image_height(buffer);
            printf("Acquired %d×%d buffer\n", width, height);

            if (argc > 1) {
                CMRawHeader cmrh;
                const void *raw = cinemavi_prepare_header(buffer, &cmrh,
                        camera_make, camera_model, shutter_us, gain_dB);
                if (endswith(argv[1], ".dng"))
                    cinemavi_generate_dng(raw, &cmrh, argv[1]);
                else if (endswith(argv[1], ".tiff"))
                    cinemavi_generate_tiff(raw, &cmrh, argv[1]);
                else if (endswith(argv[1], ".cmr"))
                    cinemavi_generate_cmr(raw, &cmrh, argv[1]);
                else
                    printf("Unknown output file type.\n");
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
