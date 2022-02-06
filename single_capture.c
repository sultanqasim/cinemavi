#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <arv.h>

#include "dng.h"
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
    if (!(*error)) arv_camera_set_string(camera, "BalanceWhiteAuto", "Off", error);
    if (!(*error)) arv_camera_set_float(camera, "BalanceRatio", 1.0, error);
}

static void cinemavi_generate_dng(ArvBuffer *buffer, char *fname)
{
    int dng_stat = arv_buffer_to_dng(buffer, fname, "");
    if (dng_stat != 0) printf("Error %d writing DNG.\n", dng_stat);
    else printf("DNG written to: %s\n", fname);
}

static void cinemavi_generate_tiff(ArvBuffer *buffer, char *fname)
{
    unsigned int width = arv_buffer_get_image_width(buffer);
    unsigned int height = arv_buffer_get_image_height(buffer);
    size_t imsz;
    const uint8_t *imbuf = (const uint8_t *) arv_buffer_get_data(buffer, &imsz);
    uint8_t *rgb8 = (uint8_t *)malloc(width * height * 3);
    if (rgb8 != NULL) {
        ImagePipelineParams params = {
            .warmth = 0.0,
            .tint = 0.15,
            .hue = 0.0,
            .sat = 1.0,
            .nr_lum = 250.0,
            .nr_chrom = 500.0,
            .gamma = 0.4,
            .shadow = 0.8
        };
        printf("Processing image...\n");
        pipeline_process_image(imbuf, rgb8, width, height, &params);
        printf("Image processed.\n");

        int tiff_stat = rgb8_to_tiff(rgb8, width, height, fname);
        if (tiff_stat != 0) printf("Error %d writing TIFF.\n", tiff_stat);
        else printf("TIFF written to: %s\n", fname);
    }
    free(rgb8);
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
        const char *camera_model = arv_camera_get_model_name(camera, NULL);
        printf("Found camera '%s'\n", camera_model);

        // Set configuration
        cinemavi_camera_configure(camera, 50000, 5, &error);
        if (error) goto err;

        // Acquire a single buffer
        buffer = arv_camera_acquisition(camera, 0, &error);

        if (ARV_IS_BUFFER(buffer)) {
            // Display some informations about the retrieved buffer
            unsigned int width = arv_buffer_get_image_width(buffer);
            unsigned int height = arv_buffer_get_image_height(buffer);
            printf("Acquired %d×%d buffer\n", width, height);

            if (argc > 1) {
                if (endswith(argv[1], ".dng"))
                    cinemavi_generate_dng(buffer, argv[1]);
                else if (endswith(argv[1], ".tiff"))
                    cinemavi_generate_tiff(buffer, argv[1]);
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
