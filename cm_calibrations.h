#ifndef CM_CALIBRATIONS_H
#define CM_CALIBRATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "colour_xfrm.h"

typedef enum {
    CMCAL_SONY_PREGIUS_GEN2,
    CMCAL_NUM_CALIBRATIONS
} CMCameraCalibrationID;

extern const ColourMatrix CM_cam_calibs[CMCAL_NUM_CALIBRATIONS];

#ifdef __cplusplus
}
#endif

#endif // CM_CALIBRATIONS_H
