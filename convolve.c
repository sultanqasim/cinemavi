#include "convolve.h"
#include <math.h>

// kernel should be an n*n array representing a square convolution kernel
// c is the variance
void gaussian_kernel(float *kernel, unsigned int n, float c)
{
    float corner = (n - 1) / -2.0;
    for (unsigned int i = 0; i < n; i++) {
        for (unsigned int j = 0; j < n; j++) {
            float x = corner + j;
            float y = corner + i;
            kernel[i*n + j] = exp((x*x + y*y) / (-2 * c*c));
        }
    }
}

static inline float convolve_pixel(const float *img, unsigned int width, unsigned int height,
        const float *kernel, unsigned int n, unsigned int x, unsigned int y, unsigned int chan)
{
    // assume n is odd
    // let k = (n - 1) / 2
    // assume k <= x < width - k
    // assume j <= y < height - k
    // assume 0 <= chan < 3

    unsigned int k = (n-1) >> 1;
    unsigned int x_base = x - k;
    unsigned int y_base = y - k;
    float s = 0;
    (void)height; // suppress unussed warning

    for (unsigned int i = 0; i < n; i++) {
        for (unsigned int j = 0; j < n; j++) {
            s += img[image_idx(x_base + j, y_base + i, chan, width)] * kernel[i*n + j];
        }
    }

    return s;
}

static inline int bounded_idx(int idx, int min, int max)
{
    if (idx < min) return min;
    if (idx >= max) return max - 1;
    return idx;
}

static inline float convolve_pixel_edge(const float *img, unsigned int width, unsigned int height,
        const float *kernel, unsigned int n, unsigned int x, unsigned int y, unsigned int chan)
{
    // assume n is odd
    unsigned int k = (n-1) >> 1;
    int x_base = x - k;
    int y_base = y - k;
    float s = 0;

    for (unsigned int i = 0; i < n; i++) {
        for (unsigned int j = 0; j < n; j++) {
            s += img[image_idx(bounded_idx(x_base + j, 0, width),
                               bounded_idx(y_base + i, 0, height),
                               chan, width)]
                * kernel[i*n + j];
        }
    }

    return s;
}

// img_out and img_in have same dimensions (width * height * 3)
// kernel is n*n, n being odd
// pad edges of input image by repeating corners
void convolve_img(const float *img_in, float *img_out, unsigned int width, unsigned int height,
        const float *kernel, unsigned int n)
{
    unsigned int k = (n-1) >> 1;

    // top edge
    for (unsigned int y = 0; y < k; y++) {
        for (unsigned int x = 0; x < width; x++) {
            for (unsigned int chan = 0; chan < 3; chan++) {
                img_out[image_idx(x, y, chan, width)] =
                    convolve_pixel_edge(img_in, width, height, kernel, n, x, y, chan);
            }
        }
    }

    // left edge
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = 0; x < k; x++) {
            for (unsigned int chan = 0; chan < 3; chan++) {
                img_out[image_idx(x, y, chan, width)] =
                    convolve_pixel_edge(img_in, width, height, kernel, n, x, y, chan);
            }
        }
    }

    // right edge
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = width - k; x < width; x++) {
            for (unsigned int chan = 0; chan < 3; chan++) {
                img_out[image_idx(x, y, chan, width)] =
                    convolve_pixel_edge(img_in, width, height, kernel, n, x, y, chan);
            }
        }
    }

    // bottom edge
    for (unsigned int y = height - k; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            for (unsigned int chan = 0; chan < 3; chan++) {
                img_out[image_idx(x, y, chan, width)] =
                    convolve_pixel_edge(img_in, width, height, kernel, n, x, y, chan);
            }
        }
    }

    // inside
    for (unsigned int y = k; y < height - k; y++) {
        for (unsigned int x = k; x < width - k; x++) {
            for (unsigned int chan = 0; chan < 3; chan++) {
                img_out[image_idx(x, y, chan, width)] =
                    convolve_pixel(img_in, width, height, kernel, n, x, y, chan);
            }
        }
    }
}
