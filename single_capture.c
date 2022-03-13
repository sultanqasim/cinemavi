#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <arv.h>

#include "cm_cli_helper.h"
#include "cm_camera_helper.h"
#include "cmraw.h"
#include "auto_exposure.h"
#include "debayer.h"
#include "pipeline.h"

static int cinemavi_auto_exposure(ArvCamera *camera, const ExposureLimits *limits,
        double *shutter, double *gain)
{
    GError *error = NULL;
    ExposureParams params = {.shutter_us = 1000, .gain_dB = 5};
    ExposureParams params2;
    double change_factor = 0.0;
    int ret = 0;
    int iterations = 0;

    cinemavi_camera_configure_exposure(camera, params.shutter_us, params.gain_dB, &error);

    while (iterations < 10 && (change_factor < 0.95 || change_factor > 1.05)) {
        if (error) {
            ret = -EIO;
            break;
        }

        // Acquire a single buffer
        ArvBuffer *buffer = arv_camera_acquisition(camera, 0, &error);

        if (!ARV_IS_BUFFER(buffer)) {
            ret = -EIO;
            break;
        }

        ArvPixelFormat pfmt = arv_buffer_get_image_pixel_format(buffer);
        if (pfmt != ARV_PIXEL_FORMAT_BAYER_RG_12P) {
            ret = -EINVAL;
            g_clear_object(&buffer);
            break;
        }

        unsigned int width = arv_buffer_get_image_width(buffer);
        unsigned int height = arv_buffer_get_image_height(buffer);
        uint16_t rgw = width >> 1;
        uint16_t rgh = height >> 1;


        size_t imsz;
        const void *imbuf = (const void *)arv_buffer_get_data(buffer, &imsz);

        uint16_t *bayer12 = (uint16_t *)malloc(width * height * sizeof(uint16_t));
        if (bayer12 == NULL) {
            ret = -ENOMEM;
            g_clear_object(&buffer);
            break;
        }
        unpack12_16(bayer12, imbuf, width * height, false);
        g_clear_object(&buffer);

        uint16_t *img_rgb = (uint16_t *)malloc(rgw * rgh * 3 * sizeof(uint16_t));
        if (img_rgb == NULL) {
            ret = -ENOMEM;
            free(bayer12);
            break;
        }

        debayer22_binned(bayer12, img_rgb, width, height);
        free(bayer12);

        change_factor = auto_exposure(img_rgb, rgw, rgh, 1300, 3700, 4090);
        free(img_rgb);

        calculate_exposure(&params, &params2, limits, change_factor);
        params = params2;
        cinemavi_camera_configure_exposure(camera, params.shutter_us, params.gain_dB, &error);
        iterations++;
    }

    *shutter = params.shutter_us;
    *gain = params.gain_dB;

    return ret;
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

        // Initial configuration
        cinemavi_camera_configure(camera, &error);
        if (error) goto err;

        // Auto exposure settings
        ExposureLimits limits;
        limits.shutter_targ_low = 8000;
        limits.shutter_targ_high = 30000;
        limits.gain_targ_low = 5;
        limits.gain_targ_high = 15;
        if (!error) arv_camera_get_gain_bounds(camera, &limits.gain_min, &limits.gain_max, &error);
        if (!error) arv_camera_get_exposure_time_bounds(camera, &limits.shutter_min, &limits.shutter_max, &error);
        if (error) goto err;

        // Determine exposure
        double shutter_us;
        double gain_dB;
        printf("Running auto exposure...\n");
        if (cinemavi_auto_exposure(camera, &limits, &shutter_us, &gain_dB)) {
            printf("Auto exposure failed.\n");
            goto err;
        }

        printf("Shutter: %.1f us Gain: %.2f dB\n", shutter_us, gain_dB);
        cinemavi_camera_configure_exposure(camera, shutter_us, gain_dB, &error);
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

                // automatic as-shot white balance
                double temp_K, tint, white_x, white_y;
                CMAutoWhiteParams awbp = {.awb_mode=CMWHITE_ROBUST};
                pipeline_auto_white_balance(raw, &cmrh.cinfo, &awbp, &temp_K, &tint);
                colour_temp_tint_to_xy(temp_K, tint, &white_x, &white_y);
                cmrh.cinfo.white_x = white_x;
                cmrh.cinfo.white_y = white_y;

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
