#ifndef CONVOLVE_H
#define CONVOLVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// kernel should be an n*n array representing a square convolution kernel
// c is the variance
void gaussian_kernel(float *kernel, unsigned int n, float c);

// img_out and img_in have same dimensions (width * height * 3)
// kernel is n*n, n being odd
// pad edges of input image by repeating corners
void convolve_img(const float *img_in, float *img_out, unsigned int width, unsigned int height,
        const float *kernel, unsigned int n);

static inline unsigned int image_idx(unsigned int x, unsigned int y, unsigned int chan, unsigned int width)
{
    return 3 * (x + y*width) + chan;
}

// returns requested percentile (0.0 to 1.0) of supplied array
// does partial sorting in-place, clobbering array in the process
float percentile_float_inplace(float *scratch, size_t num, double p);

// returns median of supplied array
float median_float(const float *arr, size_t num);

// median value in (2k+1) x (2k+1) square centred at (x, y) for selected channel
// bounds checking is performed, repeating edge pixels
float median_pixel_edge(const float *img, unsigned int width, unsigned int height,
        unsigned int k, unsigned int x, unsigned int y, unsigned int chan);

// similar to above, but optimized for specific square sizes and no edge bounds checking
float median_pixel_33(const float *img, unsigned int width, unsigned int height,
        unsigned int x, unsigned int y, unsigned int chan);
float median_pixel_55(const float *img, unsigned int width, unsigned int height,
        unsigned int x, unsigned int y, unsigned int chan);
float median_pixel_77(const float *img, unsigned int width, unsigned int height,
        unsigned int x, unsigned int y, unsigned int chan);

#ifdef __cplusplus
}
#endif

#endif
