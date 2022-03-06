#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "pipeline.h"
#include "cmraw.h"
#include "debayer.h"
#include "noise_reduction.h"
#include "gamma.h"
#include "auto_exposure.h"

static void pipeline_gen_lut(uint8_t *glut, CMLUTMode lut_mode, double black_point,
        double gamma, double shadow, double black)
{
    switch (lut_mode) {
    case CMLUT_LINEAR:
    default:
        gamma_gen_lut(glut, 12, black_point);
        break;
    case CMLUT_FILMIC:
        gamma_gen_lut_filmic(glut, 12, black_point, gamma, black);
        break;
    case CMLUT_CUBIC:
        gamma_gen_lut_cubic(glut, 12, black_point, gamma, black);
        break;
    case CMLUT_HDR:
        gamma_gen_lut_hdr(glut, 12, black_point, gamma, shadow);
        break;
    case CMLUT_HDR_CUBIC:
        gamma_gen_lut_hdr_cubic(glut, 12, black_point, gamma, shadow, black);
        break;
    }
}

static void pipeline_auto_hdr(uint16_t *rgb12, uint16_t width, uint16_t height,
        CMLUTMode *lut_mode, double *gamma, double *shadow, double *black)
{
    const double targ10 = 200;
    const double targ75 = 800;

    if (*lut_mode == CMLUT_HDR_AUTO) {
        double boost = auto_hdr_shadow(rgb12, width, height, targ10, targ75);
        if (boost < 1) boost = 1.0;
        else if (boost > 32) boost = 32.0;
        *shadow = boost;

        *lut_mode = CMLUT_HDR;
        *gamma = 0.2;
    } else if (*lut_mode == CMLUT_HDR_CUBIC_AUTO) {
        uint16_t p10, p75, p99;
        if (!exposure_percentiles(rgb12, width, height, &p10, &p75, &p99)) {
            *shadow = pow(targ75 / p75, 1.4);
            if (*shadow > 48) *shadow = 48;
            else if (*shadow < 1) *shadow = 1;

            double ln_shadow = log(*shadow);
            *gamma = 0.3 - 0.06*ln_shadow;
            if (*gamma < 0.05) *gamma = 0.05;

            double black_boost = targ10 / (p10 * *shadow);
            if (black_boost > 6) black_boost = 6;
            else if (black_boost < 1) black_boost = 1;
            *black = 0.3*black_boost + 0.2*ln_shadow;
            if (*black > 3) *black = 3;
        } else {
            *gamma = 0.3;
            *shadow = 1;
            *black = 0.3;
        }

        *lut_mode = CMLUT_HDR_CUBIC;
    }
}

int pipeline_process_image(const void *raw, uint8_t *rgb8, const CMCaptureInfo *cinfo,
        const ImagePipelineParams *params, const ColourMatrix *calib)
{
    int status = 0;
    uint16_t width = cinfo->width;
    uint16_t height = cinfo->height;
    if (width > CM_MAX_WIDTH || (width & 1) || height > CM_MAX_HEIGHT || (height & 1))
        return -EINVAL;

    uint16_t *bayer12 = (uint16_t *)malloc(width * height * sizeof(uint16_t));
    uint16_t *rgb12 = (uint16_t *)malloc(width * height * 3 * sizeof(uint16_t));
    float *rgbf_0 = (float *)malloc(width * height * 3 * sizeof(float));
    float *rgbf_1 = (float *)malloc(width * height * 3 * sizeof(float));
    uint8_t *glut = (uint8_t *)malloc(4096);

    if (bayer12 == NULL || rgb12 == NULL || rgbf_0 == NULL || rgbf_1 == NULL || glut == NULL) {
        status = -ENOMEM;
        goto cleanup;
    }

    // Step 1: Unpack and debayer the image
    if (cinfo->pixel_fmt == CM_PIXEL_FMT_BAYER_RG12P)
        unpack12_16(bayer12, raw, width * height, false);
    else if (cinfo->pixel_fmt == CM_PIXEL_FMT_BAYER_RG12)
        memcpy(bayer12, raw, width * height * sizeof(uint16_t));
    else {
        status = -EINVAL;
        goto cleanup;
    }
    debayer33(bayer12, rgb12, width, height);


    // Step 1.5: Compute auto HDR params if requested
    CMLUTMode lut_mode = params->lut_mode;
    double gamma = params->gamma;
    double shadow = params->shadow;
    double black = params->black;
    pipeline_auto_hdr(rgb12, width, height, &lut_mode, &gamma, &shadow, &black);

    // Step 2: Compute colour transformation matrix
    ColourMatrix cmat, cmat2;
    colour_matrix2(&cmat, params->temp_K, params->tint, params->hue, params->sat);
    colour_matmult33(&cmat2, &cmat, calib);
    colour_matrix_white_scale(&cmat2, params->exposure);
    ColourMatrix_f cmat_f;
    cmat_d2f(&cmat2, &cmat_f);

    // Step 3: Pre-clip, convert to float, and colour correct
    colour_pre_clip(rgb12, width, height, 4095, &cmat2);
    colour_i2f(rgb12, rgbf_0, width, height);
    colour_xfrm(rgbf_0, rgbf_1, width, height, &cmat_f);

    // Step 4: Noise reduction and convert back to integer
    if (params->nr_lum <= 1. && params->nr_chrom <= 1.) {
        colour_f2i(rgbf_1, rgb12, width, height, 4095);
    } else {
        noise_reduction_rgb2(rgbf_1, rgbf_0, width, height, params->nr_lum, params->nr_chrom);
        colour_f2i(rgbf_0, rgb12, width, height, 4095);
    }

    // Step 5: Gamma encode
    double black_point = auto_black_point(rgb12, width, height, 4095) / 3000.0;
    pipeline_gen_lut(glut, lut_mode, black_point, gamma, shadow, black);
    gamma_encode(rgb12, rgb8, width, height, glut);

cleanup:
    free(bayer12);
    free(rgb12);
    free(rgbf_0);
    free(rgbf_1);
    free(glut);

    return status;
}

// use fast 2x2 binned debayering and skip noise reduction
// output image is half height and half width
int pipeline_process_image_bin22(const void *raw, uint8_t *rgb8, const CMCaptureInfo *cinfo,
        const ImagePipelineParams *params, const ColourMatrix *calib)
{
    int status = 0;
    uint16_t width = cinfo->width;
    uint16_t height = cinfo->height;
    if (width > CM_MAX_WIDTH || (width & 1) || height > CM_MAX_HEIGHT || (height & 1))
        return -EINVAL;

    uint16_t width_out = width >> 1;
    uint16_t height_out = height >> 1;
    uint16_t *bayer12 = (uint16_t *)malloc(width * height * sizeof(uint16_t));
    uint16_t *rgb12 = (uint16_t *)malloc(width_out * height_out * 3 * sizeof(uint16_t));
    float *rgbf_0 = (float *)malloc(width_out * height_out * 3 * sizeof(float));
    float *rgbf_1 = (float *)malloc(width_out * height_out * 3 * sizeof(float));
    uint8_t *glut = (uint8_t *)malloc(4096);

    if (bayer12 == NULL || rgb12 == NULL || rgbf_0 == NULL || rgbf_1 == NULL || glut == NULL) {
        status = -ENOMEM;
        goto cleanup;
    }

    // Step 1: Unpack and debayer the image
    if (cinfo->pixel_fmt == CM_PIXEL_FMT_BAYER_RG12P)
        unpack12_16(bayer12, raw, width * height, false);
    else if (cinfo->pixel_fmt == CM_PIXEL_FMT_BAYER_RG12)
        memcpy(bayer12, raw, width * height * sizeof(uint16_t));
    else {
        status = -EINVAL;
        goto cleanup;
    }
    debayer22_binned(bayer12, rgb12, width, height);

    // For convenience's sake, repurpose width and height variables to match output from here on
    width = width_out;
    height = height_out;

    // Step 1.5: Compute auto HDR params if requested
    CMLUTMode lut_mode = params->lut_mode;
    double gamma = params->gamma;
    double shadow = params->shadow;
    double black = params->black;
    pipeline_auto_hdr(rgb12, width, height, &lut_mode, &gamma, &shadow, &black);

    // Step 2: Compute colour transformation matrix
    ColourMatrix cmat, cmat2;
    colour_matrix2(&cmat, params->temp_K, params->tint, params->hue, params->sat);
    colour_matmult33(&cmat2, &cmat, calib);
    colour_matrix_white_scale(&cmat2, params->exposure);
    ColourMatrix_f cmat_f;
    cmat_d2f(&cmat2, &cmat_f);

    // Step 3: Pre-clip, convert to float, colour correct, convert back to int
    colour_pre_clip(rgb12, width, height, 4095, &cmat2);
    colour_i2f(rgb12, rgbf_0, width, height);
    colour_xfrm(rgbf_0, rgbf_1, width, height, &cmat_f);
    colour_f2i(rgbf_1, rgb12, width, height, 4095);

    // Step 4: Gamma encode
    double black_point = auto_black_point(rgb12, width, height, 4095) / 3000.0;
    pipeline_gen_lut(glut, lut_mode, black_point, gamma, shadow, black);
    gamma_encode(rgb12, rgb8, width, height, glut);

cleanup:
    free(bayer12);
    free(rgb12);
    free(rgbf_0);
    free(rgbf_1);
    free(glut);

    return status;
}
