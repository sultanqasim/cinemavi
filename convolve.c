#include "convolve.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

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

    // now normalize for unit weight
    float sum = 0;
    for (unsigned int i = 0; i < n*n; i++)
        sum += kernel[i];
    float sum_inv = 1.0 / sum;
    for (unsigned int i = 0; i < n*n; i++)
        kernel[i] *= sum_inv;
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
    (void)height; // suppress unused warning

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
    // optimize dedicated code paths for common kernel sizes
    if (n == 3) {
        for (unsigned int y = k; y < height - k; y++) {
            for (unsigned int x = k; x < width - k; x++) {
                for (unsigned int chan = 0; chan < 3; chan++) {
                    img_out[image_idx(x, y, chan, width)] =
                        convolve_pixel(img_in, width, height, kernel, 3, x, y, chan);
                }
            }
        }
    } else if (n == 5) {
        for (unsigned int y = k; y < height - k; y++) {
            for (unsigned int x = k; x < width - k; x++) {
                for (unsigned int chan = 0; chan < 3; chan++) {
                    img_out[image_idx(x, y, chan, width)] =
                        convolve_pixel(img_in, width, height, kernel, 5, x, y, chan);
                }
            }
        }
    } else if (n == 7) {
        for (unsigned int y = k; y < height - k; y++) {
            for (unsigned int x = k; x < width - k; x++) {
                for (unsigned int chan = 0; chan < 3; chan++) {
                    img_out[image_idx(x, y, chan, width)] =
                        convolve_pixel(img_in, width, height, kernel, 7, x, y, chan);
                }
            }
        }
    } else {
        for (unsigned int y = k; y < height - k; y++) {
            for (unsigned int x = k; x < width - k; x++) {
                for (unsigned int chan = 0; chan < 3; chan++) {
                    img_out[image_idx(x, y, chan, width)] =
                        convolve_pixel(img_in, width, height, kernel, n, x, y, chan);
                }
            }
        }
    }
}

// returns requested percentile (0.0 to 1.0) of supplied array
// does partial sorting in-place, clobbering array in the process
float percentile_float_inplace(float *scratch, size_t num, double p)
{
    if (num == 0)
        return 0;
    if (num == 1)
        return scratch[0];

    size_t midx = num * p;
    if (midx >= num)
        midx = num - 1;

    size_t sorted_low = -1;    // every value before this index is less than or equal to it
    size_t sorted_high = num;  // every value after this index is greater than it

    while (1) {
        size_t eq = 0;
        size_t i = sorted_low + 1;
        size_t j = sorted_high - 1;

        // partition
        size_t part_idx = (i + j) >> 1;
        float part = scratch[part_idx];
        while (i < j) {
            while (scratch[i] < part) i++;
            while (scratch[j] > part) j--;
            if (scratch[i] == scratch[j]) {
                // special case, equal values below pivot OK
                // also implicitly means scratch[i] and scratch[j] == part
                i++;
                eq++;
            } else {
                // swap
                float t = scratch[i];
                scratch[i] = scratch[j];
                scratch[j] = t;
            }
        }

        // over the full array, we have
        // i elements <= part
        // eq elements equal to part (at least?)
        // i - eq elements < part (at most?)

        if (j == midx)
            break;
        else if ((i - eq < midx) && (i > midx)) {
            scratch[part_idx] = scratch[midx];
            scratch[midx] = part;
            break;
        } else if (j < midx)
            sorted_low = j;
        else
            sorted_high = j;
    }

    return scratch[midx];
}

// returns median of supplied array
float median_float(const float *arr, size_t num)
{
    // avoid huge VLAs
    const size_t MAX_STACK_FLOAT = 64;
    float *scratch;
    float sbuf[MAX_STACK_FLOAT];
    if (num > MAX_STACK_FLOAT) {
        scratch = (float *)malloc(num * sizeof(float));
        if (scratch == NULL) return 0;
    } else {
        scratch = sbuf;
    }

    memcpy(scratch, arr, num * sizeof(float));
    float ret = percentile_float_inplace(scratch, num, 0.5);

    if (num > MAX_STACK_FLOAT)
        free(scratch);

    return ret;
}
