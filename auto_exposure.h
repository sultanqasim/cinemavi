#ifndef AUTO_EXPOSURE_H
#define AUTO_EXPOSURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "colour_xfrm.h"

// find the 10th, 90th, and 99.5th percentile exposure values of the brightest channels
int exposure_percentiles(const uint16_t *img_rgb, uint16_t width, uint16_t height,
        uint16_t *percentile10, uint16_t *percentile90, uint16_t *percentile99);

// Returns shadow slope needed to boost shadows and midtones to target
double auto_hdr_shadow(const uint16_t *img_rgb, uint16_t width, uint16_t height,
        uint16_t percentile10, uint16_t percentile90);

/* Returns exposure multiplication factor to make the 90th percentile value of the brightest
 * channel equal to percentile90 argument. However, the returned factor would be reduced
 * if needed to ensure the 99.5th percentile of the brightest channel <= percentile99;
 */
double auto_exposure(const uint16_t *img_rgb, uint16_t width, uint16_t height,
        uint16_t percentile90, uint16_t percentile99, uint16_t white,
        const ColourPixel *cam_white);

// Returns darkest pixel value in green channel or 0.02, whichever is lower
float auto_black_point(const float *img_rgb, uint16_t width, uint16_t height);

// grey-world inspired algorithm that balances the 99.5th percentiles of each channel
// outputs: red is ratio to multiply red by, blue is ratio to multiply blue by
void auto_white_balance_brights(const float *img_rgb, uint16_t width, uint16_t height,
        double *red, double *blue);

// Huo's Robust Automatic White Balance
// outputs: red is ratio to multiply red by, blue is ratio to multiply blue by
void auto_white_balance_robust(const float *img_rgb, uint16_t width, uint16_t height,
        double *red, double *blue);

// spot white balance at specified coordinates (relative to top left)
// outputs: red is ratio to multiply red by, blue is ratio to multiply blue by
void auto_white_balance_spot(const float *img_rgb, uint16_t width, uint16_t height,
        uint16_t pos_x, uint16_t pos_y, double *red, double *blue);

typedef struct {
    double shutter_us;
    double gain_dB;
} ExposureParams;

typedef struct {
    double shutter_min;      // shortest supported shutter time (us)
    double shutter_max;      // longest supported shutter time (us)
    double shutter_targ_low; // no improvement in sharpness from further reducing shutter time
    double shutter_targ_high;// image likely to be blurry above this shutter time
    double gain_min;         // highest supported gain (dB)
    double gain_max;         // lowest supported gain (dB)
    double gain_targ_low;    // noise is not noticeable below this gain
    double gain_targ_high;   // noise gets obtrusive above this gain
} ExposureLimits;

// calculate new shutter speed and gain given supplied constraints
void calculate_exposure(const ExposureParams *e_old, ExposureParams *e_new,
        const ExposureLimits *limits, double change_factor);

#ifdef __cplusplus
}
#endif

#endif // AUTO_EXPOSURE_H
