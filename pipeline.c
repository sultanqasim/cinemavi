#include <errno.h>
#include <stdlib.h>

#include "pipeline.h"
#include "debayer.h"
#include "colour_xfrm.h"
#include "noise_reduction.h"
#include "gamma.h"

int pipeline_process_image(const uint16_t *bayer12p, uint8_t *rgb8, uint16_t width,
        uint16_t height, ImagePipelineParams *params)
{
    int status = 0;
    uint16_t *bayer12 = (uint16_t *)malloc(width * height * sizeof(uint16_t));
    uint16_t *rgb12 = (uint16_t *)malloc(width * height * 3 * sizeof(uint16_t));
    float *rgbf_0 = (float *)malloc(width * height * 3 * sizeof(float));
    float *rgbf_1 = (float *)malloc(width * height * 3 * sizeof(float));
    uint8_t *glut = (uint8_t *)malloc(4096);

    if (bayer12 == NULL || rgb12 == NULL || rgbf_0 == NULL || rgbf_1 == NULL || glut == NULL) {
        status = -ENOMEM;
        goto cleanup;
    }

    if (width >= 32768 || height >= 32768) {
        status = -EINVAL;
        goto cleanup;
    }

    // Step 1: Unpack and debayer the image
    unpack12_16(bayer12, bayer12p, width * height, false);
    debayer33(bayer12, rgb12, width, height);

    // Step 2: Convert to float and colour correct
    colour_i2f(rgb12, rgbf_0, width, height);
    ColourMatrix cmat;
    colour_matrix(&cmat, params->warmth, params->tint, params->hue, params->sat);
    ColourMatrix_f cmat_f;
    cmat_d2f(&cmat, &cmat_f);
    colour_xfrm(rgbf_0, rgbf_1, width, height, &cmat_f);

    // Step 3: Noise reduction
    noise_reduction_rgb2(rgbf_1, rgbf_0, width, height, params->nr_lum, params->nr_chrom);

    // Step 4: Convert back to integer and gamma encode
    colour_f2i(rgbf_0, rgb12, width, height, 4095);
    gamma_gen_lut(glut, 12, params->gamma);
    gamma_encode(rgb12, rgb8, width, height, glut);

cleanup:
    free(bayer12);
    free(rgb12);
    free(rgbf_0);
    free(rgbf_1);
    free(glut);

    return status;
}
