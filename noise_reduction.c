#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "noise_reduction.h"
#include "convolve.h"
#include "ycbcr.h"

#define KERNEL_SIZE 5
#define KERNEL_VARIANCE 1.3

// convolves image using 5x5 gaussian kernel
// outputs weighted average of original image and convolved image, weighted based on luminance
// luminance is (R+G+B) / sqrt(3)
// no NR applied to pixels with luminance values above intensity argument
void noise_reduction_rgb(const float *img_in, float *img_out, unsigned int width, unsigned int height,
        float intensity)
{
    float kernel[KERNEL_SIZE * KERNEL_SIZE];
    gaussian_kernel(kernel, KERNEL_SIZE, KERNEL_VARIANCE);

    float *img_smooth = (float *)malloc(width * height * 3 * sizeof(float));
    if (img_smooth == NULL) {
        // fail by doing no NR
        memcpy(img_out, img_in, width * height * 3 * sizeof(float));
        return;
    }

    convolve_img(img_in, img_smooth, width, height, kernel, KERNEL_SIZE);

    float inv_intensity = 1 / (intensity * sqrt(3));

    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            float luminance = img_in[image_idx(x, y, 0, width)] +
                img_in[image_idx(x, y, 1, width)] +
                img_in[image_idx(x, y, 2, width)];
            float local_weight = luminance * inv_intensity;
            local_weight = local_weight > 1 ? 1 : local_weight;
            float smooth_weight = 1 - local_weight;

            for (unsigned int chan = 0; chan < 3; chan++) {
                unsigned int idx = image_idx(x, y, chan, width);
                img_out[idx] = local_weight * img_in[idx] +
                    smooth_weight * img_smooth[idx];
            }
        }
    }

    free(img_smooth);
}

// similar to above, but with separate luminance and chrominance NR
// slower due to transform from RGB to YCbCr and back
void noise_reduction_rgb2(const float *img_in, float *img_out, unsigned int width, unsigned int height,
        float intensity_lum, float intensity_chrom)
{
    float *img_temp = (float *)malloc(width * height * 3 * sizeof(float));
    if (img_temp == NULL) {
        // fail by doing no NR
        memcpy(img_out, img_in, width * height * 3 * sizeof(float));
        return;
    }

    // temporarily put YCbCr original in img_out
    // img_temp will store YCbCr noise reduced image
    colour_xfrm(img_in, img_out, width, height, &CMf_sRGB2YCbCr);
    noise_reduction_ycbcr(img_out, img_temp, width, height, intensity_lum, intensity_chrom);
    colour_xfrm(img_temp, img_out, width, height, &CMf_YCbCr2sRGB);

    free(img_temp);
}

// similar to above, but faster since input and output image is YCbCr
void noise_reduction_ycbcr(const float *img_in, float *img_out, unsigned int width, unsigned int height,
        float intensity_lum, float intensity_chrom)
{
    float kernel[KERNEL_SIZE * KERNEL_SIZE];
    gaussian_kernel(kernel, KERNEL_SIZE, KERNEL_VARIANCE);

    float *img_smooth = (float *)malloc(width * height * 3 * sizeof(float));
    if (img_smooth == NULL) {
        // fail by doing no NR
        memcpy(img_out, img_in, width * height * 3 * sizeof(float));
        return;
    }

    convolve_img(img_in, img_smooth, width, height, kernel, KERNEL_SIZE);

    float inv_intensity_lum = 1 / intensity_lum;
    float inv_intensity_chrom = 1 / intensity_chrom;

    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            unsigned int idx = image_idx(x, y, 0, width);
            float luminance = img_in[idx];

            float local_weight_y = luminance * inv_intensity_lum;
            local_weight_y = local_weight_y > 1 ? 1 : local_weight_y;
            float smooth_weight_y = 1 - local_weight_y;

            float local_weight_cr = luminance * inv_intensity_chrom;
            local_weight_cr = local_weight_cr > 1 ? 1 : local_weight_cr;
            float smooth_weight_cr = 1 - local_weight_cr;

            img_out[idx] = local_weight_y * img_in[idx] +
                smooth_weight_y * img_smooth[idx];
            img_out[idx + 1] = local_weight_cr * img_in[idx + 1] +
                smooth_weight_cr * img_smooth[idx + 1];
            img_out[idx + 2] = local_weight_cr * img_in[idx + 2] +
                smooth_weight_cr * img_smooth[idx + 2];
        }
    }

    free(img_smooth);
}

// expects YCbCr or similar lum/chrom/chrom colour space
// uses 3x3 window for luminance, 7x7 for chrominance
static inline void nr_median_pixel(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, unsigned int x, unsigned int y, float thresh_lum, float thresh_chrom)
{
    if (img_in[image_idx(x, y, 0, width)] >= thresh_lum) {
        img_out[image_idx(x, y, 0, width)] = img_in[image_idx(x, y, 0, width)];
    } else {
        img_out[image_idx(x, y, 0, width)] = median_pixel_33(img_in, width, height, x, y, 0);
    }

    if (img_in[image_idx(x, y, 0, width)] >= thresh_chrom) {
        img_out[image_idx(x, y, 1, width)] = img_in[image_idx(x, y, 1, width)];
        img_out[image_idx(x, y, 2, width)] = img_in[image_idx(x, y, 2, width)];
    } else {
        img_out[image_idx(x, y, 1, width)] = median_pixel_77(img_in, width, height, x, y, 1);
        img_out[image_idx(x, y, 2, width)] = median_pixel_77(img_in, width, height, x, y, 2);
    }
}

static inline void nr_median_pixel_edge(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, unsigned int x, unsigned int y, float thresh_lum, float thresh_chrom)
{
    if (img_in[image_idx(x, y, 0, width)] >= thresh_lum) {
        img_out[image_idx(x, y, 0, width)] = img_in[image_idx(x, y, 0, width)];
    } else {
        img_out[image_idx(x, y, 0, width)] = median_pixel_edge(img_in, width, height, 1, x, y, 0);
    }

    if (img_in[image_idx(x, y, 0, width)] >= thresh_chrom) {
        img_out[image_idx(x, y, 1, width)] = img_in[image_idx(x, y, 1, width)];
        img_out[image_idx(x, y, 2, width)] = img_in[image_idx(x, y, 2, width)];
    } else {
        img_out[image_idx(x, y, 1, width)] = median_pixel_edge(img_in, width, height, 3, x, y, 1);
        img_out[image_idx(x, y, 2, width)] = median_pixel_edge(img_in, width, height, 3, x, y, 2);
    }
}

void noise_reduction_median_rgb(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom)
{
    float *img_temp = (float *)malloc(width * height * 3 * sizeof(float));
    if (img_temp == NULL) {
        // fail by doing no NR
        memcpy(img_out, img_in, width * height * 3 * sizeof(float));
        return;
    }

    // temporarily put YCbCr original in img_out
    // img_temp will store YCbCr noise reduced image
    colour_xfrm(img_in, img_out, width, height, &CMf_sRGB2YCbCr);
    noise_reduction_median_ycbcr(img_out, img_temp, width, height, thresh_lum, thresh_chrom);
    colour_xfrm(img_temp, img_out, width, height, &CMf_YCbCr2sRGB);

    free(img_temp);
}

void noise_reduction_median_ycbcr(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom)
{
    const unsigned int k = 3; // 7x7 is the largest "kernel" (median box) we use

    // top edge
    for (unsigned int y = 0; y < k; y++) {
        for (unsigned int x = 0; x < width; x++) {
            nr_median_pixel_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // left edge
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = 0; x < k; x++) {
            nr_median_pixel_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // right edge
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = width - k; x < width; x++) {
            nr_median_pixel_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // bottom edge
    for (unsigned int y = height - k; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            nr_median_pixel_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // inside
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = k; x < width - k; x++) {
            nr_median_pixel(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }
}

// expects YCbCr or similar lum/chrom/chrom colour space
// uses 3x3 window for luminance, 7x7 for chrominance
static inline void nr_median_pixel_x(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, unsigned int x, unsigned int y, float thresh_lum, float thresh_chrom)
{
    if (img_in[image_idx(x, y, 0, width)] >= thresh_lum) {
        img_out[image_idx(x, y, 0, width)] = img_in[image_idx(x, y, 0, width)];
    } else {
        img_out[image_idx(x, y, 0, width)] = median_pixel_x_33(img_in, width, height, x, y, 0);
    }

    if (img_in[image_idx(x, y, 0, width)] >= thresh_chrom) {
        img_out[image_idx(x, y, 1, width)] = img_in[image_idx(x, y, 1, width)];
        img_out[image_idx(x, y, 2, width)] = img_in[image_idx(x, y, 2, width)];
    } else {
        img_out[image_idx(x, y, 1, width)] = median_pixel_x_77(img_in, width, height, x, y, 1);
        img_out[image_idx(x, y, 2, width)] = median_pixel_x_77(img_in, width, height, x, y, 2);
    }
}

static inline void nr_median_pixel_x_edge(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, unsigned int x, unsigned int y, float thresh_lum, float thresh_chrom)
{
    if (img_in[image_idx(x, y, 0, width)] >= thresh_lum) {
        img_out[image_idx(x, y, 0, width)] = img_in[image_idx(x, y, 0, width)];
    } else {
        img_out[image_idx(x, y, 0, width)] = median_pixel_x_edge(img_in, width, height, 1, x, y, 0);
    }

    if (img_in[image_idx(x, y, 0, width)] >= thresh_chrom) {
        img_out[image_idx(x, y, 1, width)] = img_in[image_idx(x, y, 1, width)];
        img_out[image_idx(x, y, 2, width)] = img_in[image_idx(x, y, 2, width)];
    } else {
        img_out[image_idx(x, y, 1, width)] = median_pixel_x_edge(img_in, width, height, 3, x, y, 1);
        img_out[image_idx(x, y, 2, width)] = median_pixel_x_edge(img_in, width, height, 3, x, y, 2);
    }
}

void noise_reduction_median_x_rgb(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom)
{
    float *img_temp = (float *)malloc(width * height * 3 * sizeof(float));
    if (img_temp == NULL) {
        // fail by doing no NR
        memcpy(img_out, img_in, width * height * 3 * sizeof(float));
        return;
    }

    // temporarily put YCbCr original in img_out
    // img_temp will store YCbCr noise reduced image
    colour_xfrm(img_in, img_out, width, height, &CMf_sRGB2YCbCr);
    noise_reduction_median_x_ycbcr(img_out, img_temp, width, height, thresh_lum, thresh_chrom);
    colour_xfrm(img_temp, img_out, width, height, &CMf_YCbCr2sRGB);

    free(img_temp);
}

void noise_reduction_median_x_ycbcr(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom)
{
    const unsigned int k = 3; // 7x7 is the largest "kernel" (median box) we use

    // top edge
    for (unsigned int y = 0; y < k; y++) {
        for (unsigned int x = 0; x < width; x++) {
            nr_median_pixel_x_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // left edge
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = 0; x < k; x++) {
            nr_median_pixel_x_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // right edge
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = width - k; x < width; x++) {
            nr_median_pixel_x_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // bottom edge
    for (unsigned int y = height - k; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            nr_median_pixel_x_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // inside
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = k; x < width - k; x++) {
            nr_median_pixel_x(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }
}

// expects YCbCr or similar lum/chrom/chrom colour space
// uses 3x3 window for luminance, 9x9 for chrominance
static inline void nr_median_pixel_full_x(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, unsigned int x, unsigned int y, float thresh_lum, float thresh_chrom)
{
    if (img_in[image_idx(x, y, 0, width)] >= thresh_lum) {
        img_out[image_idx(x, y, 0, width)] = img_in[image_idx(x, y, 0, width)];
    } else {
        img_out[image_idx(x, y, 0, width)] = median_pixel_33(img_in, width, height, x, y, 0);
    }

    if (img_in[image_idx(x, y, 0, width)] >= thresh_chrom) {
        img_out[image_idx(x, y, 1, width)] = img_in[image_idx(x, y, 1, width)];
        img_out[image_idx(x, y, 2, width)] = img_in[image_idx(x, y, 2, width)];
    } else {
        img_out[image_idx(x, y, 1, width)] = median_pixel_full_x_99(img_in, width, height, x, y, 1);
        img_out[image_idx(x, y, 2, width)] = median_pixel_full_x_99(img_in, width, height, x, y, 2);
    }
}

static inline void nr_median_pixel_full_x_edge(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, unsigned int x, unsigned int y, float thresh_lum, float thresh_chrom)
{
    if (img_in[image_idx(x, y, 0, width)] >= thresh_lum) {
        img_out[image_idx(x, y, 0, width)] = img_in[image_idx(x, y, 0, width)];
    } else {
        img_out[image_idx(x, y, 0, width)] = median_pixel_edge(img_in, width, height, 1, x, y, 0);
    }

    if (img_in[image_idx(x, y, 0, width)] >= thresh_chrom) {
        img_out[image_idx(x, y, 1, width)] = img_in[image_idx(x, y, 1, width)];
        img_out[image_idx(x, y, 2, width)] = img_in[image_idx(x, y, 2, width)];
    } else {
        img_out[image_idx(x, y, 1, width)] = median_pixel_full_x_edge(img_in, width, height, 4, x, y, 1);
        img_out[image_idx(x, y, 2, width)] = median_pixel_full_x_edge(img_in, width, height, 4, x, y, 2);
    }
}

void noise_reduction_median_full_x_rgb(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom)
{
    float *img_temp = (float *)malloc(width * height * 3 * sizeof(float));
    if (img_temp == NULL) {
        // fail by doing no NR
        memcpy(img_out, img_in, width * height * 3 * sizeof(float));
        return;
    }

    // temporarily put YCbCr original in img_out
    // img_temp will store YCbCr noise reduced image
    colour_xfrm(img_in, img_out, width, height, &CMf_sRGB2YCbCr);
    noise_reduction_median_full_x_ycbcr(img_out, img_temp, width, height, thresh_lum, thresh_chrom);
    colour_xfrm(img_temp, img_out, width, height, &CMf_YCbCr2sRGB);

    free(img_temp);
}

void noise_reduction_median_full_x_ycbcr(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom)
{
    const unsigned int k = 4; // 9x9 is the largest "kernel" (median box) we use

    // top edge
    for (unsigned int y = 0; y < k; y++) {
        for (unsigned int x = 0; x < width; x++) {
            nr_median_pixel_full_x_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // left edge
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = 0; x < k; x++) {
            nr_median_pixel_full_x_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // right edge
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = width - k; x < width; x++) {
            nr_median_pixel_full_x_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // bottom edge
    for (unsigned int y = height - k; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            nr_median_pixel_full_x_edge(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }

    // inside
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = k; x < width - k; x++) {
            nr_median_pixel_full_x(img_in, img_out, width, height, x, y, thresh_lum, thresh_chrom);
        }
    }
}
