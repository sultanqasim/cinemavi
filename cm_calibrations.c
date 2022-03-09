#include "cm_calibrations.h"

// Matrix to convert from camera RGB to sRGB in D65 daylight illumination
const ColourMatrix CM_cam_calibs[CMCAL_NUM_CALIBRATIONS] = {
    // CMCAL_SONY_PREGIUS_GEN2
    {.m={
         1.72909, -0.56752,  0.07982,
        -0.55467,  1.39498, -0.51028,
         0.03931, -0.57808,  2.10375
    }},
};
