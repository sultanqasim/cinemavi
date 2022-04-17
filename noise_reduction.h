#ifndef NOISE_REDUCTION_H
#define NOISE_REDUCTION_H

#ifdef __cplusplus
extern "C" {
#endif

// convolves image using 7x7 gaussian kernel
// outputs weighted average of original image and convolved image, weighted based on luminance
// luminance is (R+G+B) / sqrt(3)
// no NR applied to pixels with luminance values above intensity argument
void noise_reduction_rgb(const float *img_in, float *img_out, unsigned int width, unsigned int height,
        float intensity);

// similar to above, but with separate luminance and chrominance NR
// slower due to transform from RGB to YCbCr and back
void noise_reduction_rgb2(const float *img_in, float *img_out, unsigned int width, unsigned int height,
        float intensity_lum, float intensity_chrom);

// similar to above, but faster since input and output image is YCbCr
void noise_reduction_ycbcr(const float *img_in, float *img_out, unsigned int width, unsigned int height,
        float intensity_lum, float intensity_chrom);

// median filter, 3x3 lum, 7x7 chrom
void noise_reduction_median_rgb(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom);

void noise_reduction_median_ycbcr(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom);

// 5 point "X" pattern median filter, 3x3 lum, 7x7 chrom
void noise_reduction_median_x_rgb(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom);

void noise_reduction_median_x_ycbcr(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom);

// median filter, 3x3 square lum, 9x9 "X" pattern chrom
void noise_reduction_median_full_x_rgb(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom);

void noise_reduction_median_full_x_ycbcr(const float *img_in, float *img_out, unsigned int width,
        unsigned int height, float thresh_lum, float thresh_chrom);

#ifdef __cplusplus
}
#endif

#endif // NOISE_REDUCTION_H
