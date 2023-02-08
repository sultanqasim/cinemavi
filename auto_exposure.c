#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#include "auto_exposure.h"
#include "colour_xfrm.h"
#include "ycrcg.h"

static uint16_t pick_samp_pitch(uint16_t width, uint16_t height)
{
    uint16_t samp_pitch;
    if (width < 50 || height < 50)
        samp_pitch = 1;
    else if (width < 500 || height < 500)
        samp_pitch = 10;
    else
        samp_pitch = (width + height) / 100;

    return samp_pitch;
}

static int u16_cmp(const void *a, const void *b)
{
    return *(uint16_t *)a - *(uint16_t *)b;
}

// find the 10th, 90th, and 99.5th percentile exposure values of each channel
// percentile pointer arguments should be to arrays of 3 elements
static int exposure_percentiles_rgb(const uint16_t *img_rgb, uint16_t width, uint16_t height,
        uint16_t *percentile10, uint16_t *percentile90, uint16_t *percentile99)
{
    uint16_t samp_pitch = pick_samp_pitch(width, height);
    uint16_t samp_width = width / samp_pitch;
    uint16_t samp_height = height / samp_pitch;

    uint16_t *samp_buf = (uint16_t *)malloc(samp_width * samp_height * sizeof(uint16_t));
    if (samp_buf == NULL) {
        for (int i = 0; i < 3; i++) {
            percentile10[i] = 0;
            percentile90[i] = 0;
            percentile99[i] = 0;
        }
        return -ENOMEM;
    }

    // find the percentiles for each colour channel
    for (int chan = 0; chan < 3; chan++) {
        // prepare reduced resolution sample image
        for (unsigned y = 0; y < samp_height; y++) {
            for (unsigned x = 0; x < samp_width; x++) {
                samp_buf[y*samp_width + x] = img_rgb[(y*samp_pitch*width + x*samp_pitch)*3 + chan];
            }
        }

        // sort it and extract the percentiles
        qsort(samp_buf, samp_width * samp_height, sizeof(uint16_t), u16_cmp);
        percentile10[chan] = samp_buf[(unsigned)(samp_width * samp_height * 0.1)];
        percentile90[chan] = samp_buf[(unsigned)(samp_width * samp_height * 0.9)];
        percentile99[chan] = samp_buf[(unsigned)(samp_width * samp_height * 0.995)];
    }

    free(samp_buf);
    return 0;
}

static int float_cmp(const void *a, const void *b)
{
    float c = *(float *)a - *(float *)b;
    return (c > 0) - (c < 0);
}

// find the 10th, 90th, and 99.5th percentile exposure values of each channel
// percentile pointer arguments should be to arrays of 3 elements
static int exposure_percentiles_rgb2(const float *img_rgb, uint16_t width, uint16_t height,
        float *percentile10, float *percentile90, float *percentile99)
{
    uint16_t samp_pitch = pick_samp_pitch(width, height);
    uint16_t samp_width = width / samp_pitch;
    uint16_t samp_height = height / samp_pitch;

    float *samp_buf = (float *)malloc(samp_width * samp_height * sizeof(float));
    if (samp_buf == NULL) {
        for (int i = 0; i < 3; i++) {
            percentile10[i] = 0;
            percentile90[i] = 0;
            percentile99[i] = 0;
        }
        return -ENOMEM;
    }

    // find the percentiles for each colour channel
    for (int chan = 0; chan < 3; chan++) {
        // prepare reduced resolution sample image
        for (unsigned y = 0; y < samp_height; y++) {
            for (unsigned x = 0; x < samp_width; x++) {
                samp_buf[y*samp_width + x] = img_rgb[(y*samp_pitch*width + x*samp_pitch)*3 + chan];
            }
        }

        // sort it and extract the percentiles
        qsort(samp_buf, samp_width * samp_height, sizeof(float), float_cmp);
        percentile10[chan] = samp_buf[(unsigned)(samp_width * samp_height * 0.1)];
        percentile90[chan] = samp_buf[(unsigned)(samp_width * samp_height * 0.9)];
        percentile99[chan] = samp_buf[(unsigned)(samp_width * samp_height * 0.995)];
    }

    free(samp_buf);
    return 0;
}

// find the 10th, 90th, and 99.5th percentile exposure values of the brightest channels
int exposure_percentiles(const uint16_t *img_rgb, uint16_t width, uint16_t height,
        uint16_t *percentile10, uint16_t *percentile90, uint16_t *percentile99)
{
    uint16_t p10[3], p90[3], p99[3];
    uint16_t p10_max = 0;
    uint16_t p90_max = 0;
    uint16_t p99_max = 0;

    // find the brightest of each channel
    int ret = exposure_percentiles_rgb(img_rgb, width, height, p10, p90, p99);
    if (ret == 0) {
        for (int chan = 0; chan < 3; chan++) {
            if (p10[chan] > p10_max) p10_max = p10[chan];
            if (p90[chan] > p90_max) p90_max = p90[chan];
            if (p99[chan] > p99_max) p99_max = p99[chan];
        }
    }

    *percentile10 = p10_max;
    *percentile90 = p90_max;
    *percentile99 = p99_max;

    return ret;
}

// Returns shadow slope needed to boost shadows and midtones to target
double auto_hdr_shadow(const uint16_t *img_rgb, uint16_t width, uint16_t height,
        uint16_t percentile10, uint16_t percentile90)
{
    uint16_t p10, p90, p99;
    if (exposure_percentiles(img_rgb, width, height, &p10, &p90, &p99))
        return 1.0;

    // now calculate the exposure change factors for shadows and midtones
    double gain10 = (double)percentile10 / p10;
    double gain90 = (double)percentile90 / p90;

    // even if shadows are dark, try not to over-brighten midtones
    if (gain10/gain90 > 2)
        return gain90 * 2;

    return gain10 > gain90 ? gain10 : gain90;
}

/* Returns exposure multiplication factor to make the 90th percentile value of the brightest
 * channel equal to percentile90 argument. However, the returned factor would be reduced
 * if needed to ensure the 99.5th percentile of the brightest channel <= percentile99;
 *
 * Note: this assumes green is the brightest channel in camera space
 */
double auto_exposure(const uint16_t *img_rgb, uint16_t width, uint16_t height,
        uint16_t percentile90, uint16_t percentile99, uint16_t white,
        const ColourPixel *cam_white)
{
    uint16_t p10[3], p90[3], p99[3];
    if (exposure_percentiles_rgb(img_rgb, width, height, p10, p90, p99))
        return 1.0;

    double red_factor = cam_white->p[1] / cam_white->p[0];
    double blue_factor = cam_white->p[1] / cam_white->p[2];
    p10[0] *= red_factor;
    p90[0] *= red_factor;
    p99[0] *= red_factor;
    p10[2] *= blue_factor;
    p90[2] *= blue_factor;
    p99[2] *= blue_factor;

    uint16_t p10_max = 0;
    uint16_t p90_max = 0;
    uint16_t p99_max = 0;
    for (int chan = 0; chan < 3; chan++) {
        if (p10[chan] > p10_max) p10_max = p10[chan];
        if (p90[chan] > p90_max) p90_max = p90[chan];
        if (p99[chan] > p99_max) p99_max = p99[chan];
    }

    // quickly darken if p99 is clipped
    if (p99_max >= white) return 0.5;

    // now calculate the exposure change factor based on our rules
    double gain90 = (double)percentile90 / p90_max;
    double gain99 = (double)percentile99 / p99_max;

    return gain90 < gain99 ? gain90 : gain99;
}

// Returns darkest pixel value in green channel or 0.02, whichever is lower
float auto_black_point(const float *img_rgb, uint16_t width, uint16_t height)
{
    float black = 0.02;

    // no overflow when width and height fit in CM_MAX_WIDTH and CM_MAX_HEIGHT
    for (unsigned i = 1; i < width * height * 3; i += 3) {
        if (img_rgb[i] < black) black = img_rgb[i];
    }

    return black;
}

// grey-world inspired algorithm that balances the 99.5th percentiles of each channel
// outputs: red is ratio to multiply red by, blue is ratio to multiply blue by
void auto_white_balance_brights(const float *img_rgb, uint16_t width, uint16_t height,
        double *red, double *blue)
{
    float p10[3], p90[3], p99[3];

    if (exposure_percentiles_rgb2(img_rgb, width, height, p10, p90, p99) != 0) {
        *red = 1.0;
        *blue = 1.0;
        return;
    }

    *red = p99[1] / p99[0];
    *blue = p99[1] / p99[2];
}

static float chroma_square(const ColourPixel_f *rgb)
{
    ColourPixel_f ycrcg;
    pixel_xfrm_f(rgb, &ycrcg, &CMf_RGB2YCrCg);

    // normalize for brightness to get chrominance
    ycrcg.p[1] /= ycrcg.p[0];
    ycrcg.p[2] /= ycrcg.p[0];
    return ycrcg.p[1]*ycrcg.p[1] + ycrcg.p[2]*ycrcg.p[2];
}

static unsigned grey_sum(const ColourPixel_f *img_pixel, uint16_t width, uint16_t height,
        double chroma_thresh, ColourPixel_f *colour_sum)
{
    unsigned num_pixels;
    memset(colour_sum, 0, sizeof(ColourPixel_f));

    if (chroma_thresh <= 0) {
        // do old fashioned grey world
        num_pixels = width*height;
        for (unsigned y = 0; y < height; y++) {
            for (unsigned x = 0; x < width; x++) {
                for (unsigned chan = 0; chan < 3; chan++)
                    colour_sum->p[chan] += img_pixel[y*width + x].p[chan];
            }
        }
    } else {
        // selective grey world ignoring high chrominance areas
        double thresh = chroma_thresh * chroma_thresh;
        num_pixels = 0;
        for (unsigned y = 0; y < height; y++) {
            for (unsigned x = 0; x < width; x++) {
                if (chroma_square(&img_pixel[y*width + x]) < thresh) {
                    num_pixels++;
                    for (unsigned chan = 0; chan < 3; chan++)
                        colour_sum->p[chan] += img_pixel[y*width + x].p[chan];
                }
            }
        }
    }

    return num_pixels;
}

// classic grey world algorithm
void auto_white_balance_grey_world(const float *img_rgb, uint16_t width, uint16_t height,
        double *red, double *blue)
{
    uint16_t samp_pitch = pick_samp_pitch(width, height);
    uint16_t samp_width = width / samp_pitch;
    uint16_t samp_height = height / samp_pitch;

    const size_t samp_buf_size = samp_width * samp_height * sizeof(ColourPixel_f);
    ColourPixel_f *samp_buf = (ColourPixel_f *)malloc(samp_buf_size);
    if (samp_buf == NULL) {
        *red = 1;
        *blue = 1;
        goto cleanup;
    }

    // populate samp_buf with downsampled image
    for (unsigned y = 0; y < samp_height; y++) {
        for (unsigned x = 0; x < samp_width; x++) {
            memcpy(&samp_buf[y*samp_width + x], &img_rgb[(y*samp_pitch*width + x*samp_pitch)*3],
                    sizeof(ColourPixel_f));
        }
    }

    ColourPixel_f colour_sum;
    grey_sum(samp_buf, samp_width, samp_height, 0, &colour_sum);
    *red = colour_sum.p[1] / colour_sum.p[0];
    *blue = colour_sum.p[1] / colour_sum.p[2];

cleanup:
    free(samp_buf);
}

// Huo's Robust Automatic White Balance
// https://web.stanford.edu/~sujason/ColorBalancing/robustawb.html
// outputs: red is ratio to multiply red by, blue is ratio to multiply blue by
void auto_white_balance_robust(const float *img_rgb, uint16_t width, uint16_t height,
        double *red, double *blue)
{
    uint16_t samp_pitch = pick_samp_pitch(width, height);
    uint16_t samp_width = width / samp_pitch;
    uint16_t samp_height = height / samp_pitch;

    *red = 1;
    *blue = 1;

    const size_t samp_buf_size = samp_width * samp_height * sizeof(ColourPixel_f);
    ColourPixel_f *samp_buf = (ColourPixel_f *)malloc(samp_buf_size);
    ColourPixel_f *samp_buf2 = (ColourPixel_f *)malloc(samp_buf_size);
    if (samp_buf == NULL || samp_buf2 == NULL) {
        goto cleanup;
    }

    // populate samp_buf with downsampled image
    for (unsigned y = 0; y < samp_height; y++) {
        for (unsigned x = 0; x < samp_width; x++) {
            memcpy(&samp_buf[y*samp_width + x], &img_rgb[(y*samp_pitch*width + x*samp_pitch)*3],
                    sizeof(ColourPixel_f));
        }
    }

    // Start with grey-ish world, then iteratively tighten the chroma threshold
    ColourPixel_f colour_sum;
    unsigned pixel_thresh = samp_width * samp_height * 0.05;
    double chroma_thresh = 0.8;
    unsigned num_pixels = grey_sum(samp_buf, samp_width, samp_height, chroma_thresh, &colour_sum);

    while (num_pixels > pixel_thresh) {
        *red *= colour_sum.p[1] / colour_sum.p[0];
        *blue *= colour_sum.p[1] / colour_sum.p[2];

        chroma_thresh *= 0.6;
        if (chroma_thresh < 0.1) break;

        // transform image based on this iteration
        ColourMatrix cmat;
        ColourMatrix_f cmat_f;
        colour_matrix(&cmat, *red, *blue, 0, 1);
        cmat_d2f(&cmat, &cmat_f);
        colour_xfrm((float *)samp_buf, (float *)samp_buf2, samp_width, samp_height, &cmat_f);

        // now perform grey colour calculation with current threshold
        num_pixels = grey_sum(samp_buf2, samp_width, samp_height, chroma_thresh, &colour_sum);
    }

cleanup:
    free(samp_buf);
    free(samp_buf2);
}

// spot white balance at specified coordinates (relative to top left)
// outputs: red is ratio to multiply red by, blue is ratio to multiply blue by
void auto_white_balance_spot(const float *img_rgb, uint16_t width, uint16_t height,
        uint16_t pos_x, uint16_t pos_y, double *red, double *blue)
{
    if (width < 7 || height < 7) {
        *red = 1;
        *blue = 1;
        return;
    }

    // we'll do a 7x7 pixel centred average
    if (pos_x < 3) pos_x = 3;
    if (pos_x > width - 4) pos_x = width - 4;
    if (pos_y < 3) pos_y = 3;
    if (pos_y > height - 4) pos_y = height - 4;

    uint16_t start_x = pos_x - 3;
    uint16_t start_y = pos_y - 3;
    ColourPixel_f colour_sum = {};
    for (unsigned x = start_x; x < start_x + 7; x++) {
        for (unsigned y = start_y; y < start_y + 7; y++) {
            for (unsigned chan = 0; chan < 3; chan++)
                colour_sum.p[chan] += img_rgb[(y*width + x)*3 + chan];
        }
    }

    *red = colour_sum.p[1] / colour_sum.p[0];
    *blue = colour_sum.p[1] / colour_sum.p[2];
}

// calculate new shutter speed and gain given supplied constraints
void calculate_exposure(const ExposureParams *e_old, ExposureParams *e_new,
        const ExposureLimits *limits, const ExposureLimits *targets, double change_factor)
{
    if (change_factor <= 0) { // invalid
        *e_new = *e_old;
        return;
    }

    // we'll work with linearized gains, convert back to dB at end
    const double lin_gain_min = pow(10, limits->gain_min / 20);
    const double lin_gain_max = pow(10, limits->gain_max / 20);
    const double lin_gain_targ_low = pow(10, targets->gain_min / 20);
    const double lin_gain_targ_high = pow(10, targets->gain_max / 20);

    double cf_root = sqrt(change_factor);
    double shutter = e_old->shutter_us * cf_root;
    double gain = pow(10, e_old->gain_dB / 20) * cf_root;

    // try to get gain within targets
    if (gain > lin_gain_targ_high && shutter < targets->shutter_max) {
        shutter *= gain / lin_gain_targ_high;
        gain = lin_gain_targ_high;
    } else if (gain < lin_gain_targ_low && shutter > targets->shutter_min) {
        shutter *= gain / lin_gain_targ_low;
        gain = lin_gain_targ_low;
    }

    // try to get shutter within targets
    if (shutter > targets->shutter_max && gain < lin_gain_targ_high) {
        gain *= shutter / targets->shutter_max;
        shutter = targets->shutter_max;
    } else if (shutter < targets->shutter_min && gain > lin_gain_targ_low) {
        gain *= shutter / targets->shutter_min;
        shutter = targets->shutter_min;
    }

    // balance shutter and gain when out of target bounds
    if (gain >= lin_gain_targ_high && shutter >= targets->shutter_max) {
        double gain_excess = gain / lin_gain_targ_high;
        double shutter_excess = shutter / targets->shutter_max;
        double excess_ratio = sqrt(gain_excess * shutter_excess);
        gain *= excess_ratio / gain_excess;
        shutter *= excess_ratio / shutter_excess;
    } else if (gain <= lin_gain_targ_low && shutter <= targets->shutter_min) {
        double gain_lack = lin_gain_targ_low / gain;
        double shutter_lack = targets->shutter_min / shutter;
        double lack_ratio = sqrt(gain_lack * shutter_lack);
        gain *= gain_lack / lack_ratio;
        shutter *= shutter_lack / lack_ratio;
    }

    // make sure gain is within limits
    if (gain < lin_gain_min) {
        shutter *= gain / lin_gain_min;
        gain = lin_gain_min;
    } else if (gain > lin_gain_max) {
        shutter *= gain / lin_gain_max;
        gain = lin_gain_max;
    }

    // make sure shutter is within limits
    if (shutter > limits->shutter_max) {
        shutter = limits->shutter_max;
    } else if (shutter < limits->shutter_min) {
        shutter = limits->shutter_min;
    }

    e_new->shutter_us = shutter;
    e_new->gain_dB = 20 * log10(gain);
}
