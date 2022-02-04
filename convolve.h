#ifndef CONVOLVE_H
#define CONVOLVE_H

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif
