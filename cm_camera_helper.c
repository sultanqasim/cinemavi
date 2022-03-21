#include <math.h>
#include <stdio.h>
#include <arv.h>

#include "cm_camera_helper.h"
#include "cmraw.h"

void cinemavi_camera_configure_exposure(ArvCamera *camera, double shutter_us,
        double gain_db, GError **error)
{
    if (!(*error)) arv_camera_set_gain(camera, gain_db, error);
    if (!(*error)) shutter_us *= pow(10, (gain_db - arv_camera_get_gain(camera, error)) / 20);
    if (!(*error)) arv_camera_set_exposure_time(camera, shutter_us, error);
}

void cinemavi_camera_configure(ArvCamera *camera, GError **error)
{
    int wmin, wmax, hmin, hmax;

    // Set image size to use full sensor
    if (!(*error)) arv_camera_get_width_bounds(camera, &wmin, &wmax, error);
    if (!(*error)) arv_camera_get_height_bounds(camera, &hmin, &hmax, error);
    if (!(*error)) arv_camera_set_region(camera, 0, 0, wmax, hmax, error);
    if (!(*error)) arv_camera_set_pixel_format(camera, ARV_PIXEL_FORMAT_BAYER_RG_12P, error);
    if (!(*error)) arv_camera_set_binning(camera, 1, 1, error);

    // Disable auto exposure
    if (!(*error)) arv_camera_set_gain_auto(camera, ARV_AUTO_OFF, error);
    if (!(*error)) arv_camera_set_exposure_time_auto(camera, ARV_AUTO_OFF, error);

    // Disable auto white balance and make it neutral
    if (!(*error)) arv_camera_set_string(camera, "BalanceWhiteAuto", "Continuous", error);
    if (!(*error)) arv_camera_set_string(camera, "BalanceWhiteAuto", "Off", error);
    if (!(*error)) arv_camera_set_string(camera, "BalanceRatioSelector", "Red", error);
    if (!(*error)) arv_camera_set_float(camera, "BalanceRatio", 1.0, error);
    if (!(*error)) arv_camera_set_string(camera, "BalanceRatioSelector", "Blue", error);
    if (!(*error)) arv_camera_set_float(camera, "BalanceRatio", 1.0, error);
}

const void * cinemavi_prepare_header(ArvBuffer *buffer, CMRawHeader *cmrh,
        const char *cam_make, const char *cam_model, float shutter, float gain)
{
    size_t imsz;
    const void *imbuf = (const void *)arv_buffer_get_data(buffer, &imsz);

    cm_raw_header_init(cmrh);

    if (arv_buffer_get_payload_type(buffer) != ARV_BUFFER_PAYLOAD_TYPE_IMAGE)
        return NULL;

    cmrh->cinfo.width = arv_buffer_get_image_width(buffer);
    cmrh->cinfo.height = arv_buffer_get_image_height(buffer);

    ArvPixelFormat pfmt = arv_buffer_get_image_pixel_format(buffer);
    if (pfmt == ARV_PIXEL_FORMAT_BAYER_RG_12P)
        cmrh->cinfo.pixel_fmt = CM_PIXEL_FMT_BAYER_RG12P;
    else if (pfmt == ARV_PIXEL_FORMAT_BAYER_RG_12)
        cmrh->cinfo.pixel_fmt = CM_PIXEL_FMT_BAYER_RG12;
    else
        return NULL;

    if (cam_make != NULL)
        snprintf(cmrh->camera_make, sizeof(cmrh->camera_make), "%s", cam_make);
    if (cam_model != NULL)
        snprintf(cmrh->camera_model, sizeof(cmrh->camera_model), "%s", cam_model);
    snprintf(cmrh->capture_software, sizeof(cmrh->capture_software), "Cinemavi");

    cmrh->cinfo.shutter_us = shutter;
    cmrh->cinfo.gain_dB = gain;

    // hard code for now
    cmrh->cinfo.focal_len_mm = 8.0;

    return imbuf;
}
