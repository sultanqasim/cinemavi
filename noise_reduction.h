#ifndef NOISE_REDUCTION_H
#define NOISE_REDUCTION_H

#ifdef __cplusplus
extern "C" {
#endif

// convolves image using 7x7 gaussian kernel
// outputs weighted average of original image and convolved image, weighted based on luminance
// luminance is (R+G+B) / 3
// no NR applied to pixels with luminance values above intensity argument
void noise_reduction_rgb(const float *img_in, float *img_out, unsigned int width, unsigned int height,
        float intensity);

#ifdef __cplusplus
}
#endif

#endif // NOISE_REDUCTION_H
