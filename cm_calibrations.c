#include "cm_calibrations.h"

// Matrix to convert from camera RGB to sRGB in D65 daylight illumination
const ColourMatrix CM_cam_calibs[CMCAL_NUM_CALIBRATIONS] = {
    // CMCAL_SONY_PREGIUS_GEN2
    {.m={
         1.75883, -0.68132,  0.01113,
        -0.58876,  1.49340, -0.55559,
         0.04679, -0.59206,  2.02246
    }},
};
