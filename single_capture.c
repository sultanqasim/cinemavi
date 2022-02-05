#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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
            unsigned int width = arv_buffer_get_image_width(buffer);
            unsigned int height = arv_buffer_get_image_height(buffer);

            // Display some informations about the retrieved buffer
            printf("Acquired %d×%d buffer\n", width, height);

            if (argc > 1) {
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
                    };
                    printf("Processing image...\n");
                    pipeline_process_image(imbuf, rgb8, width, height, &params);
                    printf("Image processed.\n");

                    int tiff_stat = rgb8_to_tiff(rgb8, width, height, argv[1]);
                    if (tiff_stat != 0) printf("Error %d writing TIFF.\n", tiff_stat);
                    else printf("TIFF written to: %s\n", argv[1]);
                }
                free(rgb8);
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
