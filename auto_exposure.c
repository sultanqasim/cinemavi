#include <stdlib.h>
#include <math.h>

#include "auto_exposure.h"

static int u16_cmp(const void *a, const void *b)
{
    return *(uint16_t *)a - *(uint16_t *)b;
}

/* Returns exposure multiplication factor to make the 70th percentile value of the brightest
 * channel equal to percentile70 argument. However, the returned factor would be reduced
 * if needed to ensure the 98th percentile of the brightest channel <= percentile98;
 */
double auto_exposure(const uint16_t *img_rgb, uint16_t width, uint16_t height,
        uint16_t percentile70, uint16_t percentile98)
{
    uint16_t samp_pitch;
    if (width < 50 || height < 50)
        samp_pitch = 1;
    else if (width < 500 || height < 500)
        samp_pitch = 10;
    else
        samp_pitch = 50;

    uint16_t samp_width = width / samp_pitch;
    uint16_t samp_height = height / samp_pitch;

    uint16_t *samp_buf = (uint16_t *)malloc(samp_width * samp_height * sizeof(uint16_t));
    if (samp_buf == NULL)
        return 1.0;

    uint16_t p70_max = 0;
    uint16_t p98_max = 0;

    // calculate 50th and 90th percentiles of each colour channel
    for (int chan = 0; chan < 3; chan++) {
        // prepare reduced resolution sample image
        for (unsigned y = 0; y < samp_height; y++) {
            for (unsigned x = 0; x < samp_width; x++) {
                samp_buf[y*samp_width + x] = img_rgb[(y*samp_pitch*width + x*samp_pitch)*3 + chan];
            }
        }

        // sort it and extract the percentiles
        qsort(samp_buf, samp_width * samp_height, sizeof(uint16_t), u16_cmp);
        uint16_t p70 = samp_buf[(unsigned)(samp_width * samp_height * 0.7)];
        uint16_t p98 = samp_buf[(unsigned)(samp_width * samp_height * 0.98)];

        if (p70 > p70_max) p70_max = p70;
        if (p98 > p98_max) p98_max = p98;
    }

    free(samp_buf);

    // now calculate the exposure change factor based on our rules
    double gain50 = (double)percentile70 / p70_max;
    double gain90 = (double)percentile98 / p98_max;

    return gain50 < gain90 ? gain50 : gain90;
}

// calculate new shutter speed and gain given supplied constraints
void calculate_exposure(const ExposureParams *e_old, ExposureParams *e_new,
        const ExposureLimits *limits, double change_factor)
{
    if (change_factor <= 0) { // invalid
        *e_new = *e_old;
        return;
    }

    // we'll work with linearized gains, convert back to dB at end
    const double lin_gain_min = pow(10, limits->gain_min / 20);
    const double lin_gain_max = pow(10, limits->gain_max / 20);
    const double lin_gain_targ_low = pow(10, limits->gain_targ_low / 20);
    const double lin_gain_targ_high = pow(10, limits->gain_targ_high / 20);

    double cf_root = sqrt(change_factor);
    double shutter = e_old->shutter_us * cf_root;
    double gain = pow(10, e_old->gain_dB / 20) * cf_root;

    // try to get gain within targets
    if (gain > lin_gain_targ_high && shutter < limits->shutter_targ_high) {
        shutter *= gain / lin_gain_targ_high;
        gain = lin_gain_targ_high;
    } else if (gain < lin_gain_targ_low && shutter > limits->shutter_targ_low) {
        shutter *= gain / lin_gain_targ_low;
        gain = lin_gain_targ_low;
    }

    // try to get shutter within targets
    if (shutter > limits->shutter_targ_high && gain < lin_gain_targ_high) {
        gain *= shutter / limits->shutter_targ_high;
        shutter = limits->shutter_targ_high;
    } else if (shutter < limits->shutter_targ_low && gain > lin_gain_targ_low) {
        gain *= shutter / limits->shutter_targ_low;
        shutter = limits->shutter_targ_low;
    }

    // balance shutter and gain when out of target bounds
    if (gain >= lin_gain_targ_high && shutter >= limits->shutter_targ_high) {
        double gain_excess = gain / lin_gain_targ_high;
        double shutter_excess = shutter / limits->shutter_targ_high;
        double excess_ratio = sqrt(gain_excess * shutter_excess);
        gain *= excess_ratio / gain_excess;
        shutter *= excess_ratio / shutter_excess;
    } else if (gain <= lin_gain_targ_low && shutter <= limits->shutter_targ_low) {
        double gain_lack = lin_gain_targ_low / gain;
        double shutter_lack = limits->shutter_targ_low / shutter;
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
