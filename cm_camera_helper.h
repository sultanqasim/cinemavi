#ifndef CM_CAMERA_HELPER_H
#define CM_CAMERA_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <arv.h>
#include "cmraw.h"

void cinemavi_camera_configure_exposure(ArvCamera *camera, double shutter_us,
        double gain_db, GError **error);

void cinemavi_camera_configure(ArvCamera *camera, GError **error);

const void * cinemavi_prepare_header(ArvBuffer *buffer, CMRawHeader *cmrh,
        const char *cam_make, const char *cam_model, float shutter, float gain);

#ifdef __cplusplus
}
#endif

#endif // CM_CAMERA_HELPER_H
