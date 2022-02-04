#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "noise_reduction.h"
#include "convolve.h"

// convolves image using 7x7 gaussian kernel
// outputs weighted average of original image and convolved image, weighted based on luminance
// luminance is (R+G+B) / 3
// no NR applied to pixels with luminance values above intensity argument
#define KERNEL_SIZE 7
void noise_reduction_rgb(const float *img_in, float *img_out, unsigned int width, unsigned int height,
        float intensity)
{
    float kernel[KERNEL_SIZE * KERNEL_SIZE];
    gaussian_kernel(kernel, KERNEL_SIZE, 1.41421);

    float *img_smooth = (float *)malloc(width * height * 3 * sizeof(float));
    if (img_smooth == NULL) {
        // fail by doing no NR
        memcpy(img_out, img_in, width * height * 3 * sizeof(float));
        return;
    }

    convolve_img(img_in, img_smooth, width, height, kernel, KERNEL_SIZE);

    float inv_intensity = 1.0 / (3 * intensity);

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
