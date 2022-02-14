#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cmraw.h"
#include "pipeline.h"
#include "debayer.h"
#include "colour_xfrm.h"
#include "noise_reduction.h"
#include "gamma.h"
#include "auto_exposure.h"

static void pipeline_gen_lut(uint8_t *glut, CMLUTMode lut_mode, double gamma, double shadow)
{
    switch (lut_mode) {
    case CMLUT_LINEAR:
    default:
        gamma_gen_lut(glut, 12);
        break;
    case CMLUT_FILMIC:
        gamma_gen_lut_filmic(glut, 12, gamma, shadow);
        break;
    case CMLUT_CUBIC:
        gamma_gen_lut_cubic(glut, 12, gamma, shadow);
        break;
    case CMLUT_HDR:
        gamma_gen_lut_hdr(glut, 12, gamma, shadow);
        break;
    }
}

static void pipeline_auto_hdr(uint16_t *rgb12, uint16_t width, uint16_t height,
        CMLUTMode *lut_mode, double *gamma, double *shadow)
{
    if (*lut_mode == CMLUT_HDR_AUTO) {
        double boost = auto_hdr_shadow(rgb12, width, height, 360, 1800);
        if (boost > 2) {
            *lut_mode = CMLUT_HDR;
            *shadow = boost > 32 ? 32 : boost;
            *gamma = 0.3 - pow(*shadow, 1./3)*0.06;
        } else {
            *lut_mode = CMLUT_CUBIC;
        }
    }
}

int pipeline_process_image(const void *raw, uint8_t *rgb8, const CMCaptureInfo *cinfo,
        const ImagePipelineParams *params, const CMCameraCalibration *calib)
{
    int status = 0;
    uint16_t width = cinfo->width;
    uint16_t height = cinfo->height;
    uint16_t *bayer12 = (uint16_t *)malloc(width * height * sizeof(uint16_t));
    uint16_t *rgb12 = (uint16_t *)malloc(width * height * 3 * sizeof(uint16_t));
    float *rgbf_0 = (float *)malloc(width * height * 3 * sizeof(float));
    float *rgbf_1 = (float *)malloc(width * height * 3 * sizeof(float));
    uint8_t *glut = (uint8_t *)malloc(4096);

    if (bayer12 == NULL || rgb12 == NULL || rgbf_0 == NULL || rgbf_1 == NULL || glut == NULL) {
        status = -ENOMEM;
        goto cleanup;
    }

    if (width > CM_MAX_WIDTH || (width & 1) || height > CM_MAX_HEIGHT || (height & 1)) {
        status = -EINVAL;
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
    pipeline_auto_hdr(rgb12, width, height, &lut_mode, &gamma, &shadow);

    // Step 2: Convert to float and colour correct
    colour_i2f(rgb12, rgbf_0, width, height);
    ColourMatrix cmat;
    colour_matrix(&cmat, params->exposure, params->warmth + calib->warmth,
            params->tint + calib->tint, params->hue + calib->hue, params->sat);
    ColourMatrix_f cmat_f;
    cmat_d2f(&cmat, &cmat_f);
    colour_xfrm(rgbf_0, rgbf_1, width, height, &cmat_f);

    // Step 3: Noise reduction and convert back to integer
    if (params->nr_lum <= 1. && params->nr_chrom <= 1.) {
        colour_f2i(rgbf_1, rgb12, width, height, 4095);
    } else {
        noise_reduction_rgb2(rgbf_1, rgbf_0, width, height, params->nr_lum, params->nr_chrom);
        colour_f2i(rgbf_0, rgb12, width, height, 4095);
    }

    // Step 4: Gamma encode
    pipeline_gen_lut(glut, lut_mode, gamma, shadow);
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
        const ImagePipelineParams *params, const CMCameraCalibration *calib)
{
    int status = 0;
    uint16_t width = cinfo->width;
    uint16_t height = cinfo->height;
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

    if (width >= 32768 || (width & 1) || height >= 32768 || (height & 1)) {
        status = -EINVAL;
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

    // Step 1.5: Compute auto HDR params if requested
    CMLUTMode lut_mode = params->lut_mode;
    double gamma = params->gamma;
    double shadow = params->shadow;
    pipeline_auto_hdr(rgb12, width, height, &lut_mode, &gamma, &shadow);

    // Step 2: Convert to float, colour correct, convert back to int
    colour_i2f(rgb12, rgbf_0, width_out, height_out);
    ColourMatrix cmat;
    colour_matrix(&cmat, params->exposure, params->warmth + calib->warmth,
            params->tint + calib->tint, params->hue + calib->hue, params->sat);
    ColourMatrix_f cmat_f;
    cmat_d2f(&cmat, &cmat_f);
    colour_xfrm(rgbf_0, rgbf_1, width_out, height_out, &cmat_f);
    colour_f2i(rgbf_1, rgb12, width_out, height_out, 4095);

    // Step 3: Gamma encode
    pipeline_gen_lut(glut, lut_mode, gamma, shadow);
    gamma_encode(rgb12, rgb8, width_out, height_out, glut);

cleanup:
    free(bayer12);
    free(rgb12);
    free(rgbf_0);
    free(rgbf_1);
    free(glut);

    return status;
}
