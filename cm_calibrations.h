#ifndef CM_CALIBRATIONS_H
#define CM_CALIBRATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "colour_xfrm.h"
#include "cmraw.h"

typedef enum {
    CMCAL_SONY_PREGIUS_GEN2,
    CMCAL_NUM_CALIBRATIONS
} CMCameraCalibrationID;

extern const ColourMatrix CM_cam_calibs[CMCAL_NUM_CALIBRATIONS];

inline static const ColourMatrix * get_calibration(const CMCaptureInfo *cinfo)
{
    uint16_t calib_index = cinfo->calib_id;
    if (calib_index >= CMCAL_NUM_CALIBRATIONS) calib_index = 0;
    return &CM_cam_calibs[calib_index];
}

#ifdef __cplusplus
}
#endif

#endif // CM_CALIBRATIONS_H
