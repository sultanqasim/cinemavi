#include "cm_calibrations.h"

// Matrix to convert from camera RGB to sRGB in D65 daylight illumination
const ColourMatrix CM_cam_calibs[CMCAL_NUM_CALIBRATIONS] = {
    // CMCAL_SONY_PREGIUS_GEN2
    {.m={
         1.57977468, -0.38016131, -0.07207294,
        -0.51368900,  1.16489037, -0.22366352,
        -0.16467965, -0.30573312,  1.89879966
    }},
};
