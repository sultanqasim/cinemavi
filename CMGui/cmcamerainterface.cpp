#include "cmcamerainterface.h"
#include "../cm_camera_helper.h"
#include <QMutexLocker>

CMCameraInterface::CMCameraInterface()
{
    // run captureLoop in its own thread
    this->moveToThread(&captureThread);
    connect(&captureThread, &QThread::started, this, &CMCameraInterface::captureLoop);
}

CMCameraInterface::~CMCameraInterface()
{
    this->stopCapture();
    if (this->camera) {
        g_clear_object(&this->stream);
        g_clear_object(&this->camera);
    }
}

bool CMCameraInterface::cameraAvailable()
{
    if (ARV_IS_CAMERA(this->camera))
        return true;

    GError *error = NULL;
    this->camera = arv_camera_new(NULL, &error);

    if (ARV_IS_CAMERA(this->camera)) {
        if (!error)
            this->cameraMake = arv_camera_get_vendor_name(this->camera, &error);
        if (!error)
            this->cameraModel = arv_camera_get_model_name(this->camera, &error);
        if (!error)
            arv_camera_get_gain_bounds(this->camera, &this->gainMin, &this->gainMax, &error);
        if (!error)
            arv_camera_get_exposure_time_bounds(this->camera, &this->shutterMin, &this->shutterMax, &error);

        // Turn off in-camera auto exposure and auto white balance, capture full frame
        if (!error)
            cinemavi_camera_configure(this->camera, &error);

        if (!error)
            arv_camera_get_frame_rate_bounds(this->camera, &this->frameRateMin,
                                             &this->frameRateMax, &error);

        // Stream initialization
        this->stream = NULL;
        if (!error)
            this->stream = arv_camera_create_stream (this->camera, NULL, NULL, &error);
        if (!error && ARV_IS_STREAM(this->stream)) {
            size_t payload = arv_camera_get_payload(this->camera, &error);
            if (!error) {
                for (int i = 0; i < 5; i++)
                    arv_stream_push_buffer(this->stream, arv_buffer_new(payload, NULL));
            }
        }

        if (error) {
            g_clear_object(&this->camera);
            g_clear_object(&this->stream);
            return false;
        }

        return true;
    } else
        return false;
}

void CMCameraInterface::setFrameRate(double frameRate)
{
    if (!ARV_IS_CAMERA(this->camera)) return;

    if (frameRate > this->frameRateMax)
        frameRate = this->frameRateMax;
    else if (frameRate < this->frameRateMin)
        frameRate = this->frameRateMin;

    GError *error = NULL;
    arv_camera_set_frame_rate(this->camera, frameRate, &error);
    if (error)
        g_clear_object(&this->camera);
}

void CMCameraInterface::setExposure(double shutter_us, double gain_dB)
{
    if (!ARV_IS_CAMERA(this->camera)) return;

    if (shutter_us > this->shutterMax)
        shutter_us = this->shutterMax;
    else if (shutter_us < this->shutterMin)
        shutter_us = this->shutterMin;

    if (gain_dB > this->gainMax)
        gain_dB = this->gainMax;
    else if (gain_dB < this->gainMin)
        gain_dB = this->gainMin;

    GError *error = NULL;
    cinemavi_camera_configure_exposure(this->camera, shutter_us, gain_dB, &error);
    if (error)
        g_clear_object(&this->camera);
    else {
        this->shutter = shutter_us;
        this->gain = gain_dB;
    }
}

void CMCameraInterface::updateExposure(double changeFactor)
{
    ExposureLimits limits;
    limits.shutter_targ_low = 8000;
    limits.shutter_targ_high = 30000;
    limits.gain_targ_low = 5;
    limits.gain_targ_high = 15;
    limits.shutter_min = this->shutterMin;
    limits.shutter_max = 250000; // minimum 4 fps
    limits.gain_min = this->gainMin;
    limits.gain_max = this->gainMax;

    ExposureParams currentExp = {.shutter_us = this->shutter, .gain_dB = this->gain};
    ExposureParams newExp;
    calculate_exposure(&currentExp, &newExp, &limits, changeFactor);
    this->setExposure(newExp.shutter_us, newExp.gain_dB);
}

void CMCameraInterface::startCapture()
{
    GError *error = NULL;
    if (!ARV_IS_CAMERA(this->camera)) return;
    if (this->capturing) return;

    arv_camera_start_acquisition (this->camera, &error);
    if (error)
        g_clear_object(&this->camera);
    else {
        this->capturing = true;
        this->captureThread.start();
    }
}

void CMCameraInterface::stopCapture()
{
    GError *error = NULL;
    if (!ARV_IS_CAMERA(this->camera)) return;
    this->capturing = false;
    arv_camera_stop_acquisition(this->camera, &error);

    captureThread.quit();
    captureThread.wait();
}

void CMCameraInterface::captureLoop()
{
    while (this->capturing) {
        ArvBuffer *buf = arv_stream_pop_buffer(this->stream);
        if (ARV_IS_BUFFER(buf)) {
            CMRawImage img;
            CMCaptureInfo cinfo = {};
            cinfo.width = arv_buffer_get_image_width(buf);
            cinfo.height = arv_buffer_get_image_height(buf);
            cinfo.shutter_us = this->shutter;
            cinfo.gain_dB = this->gain;
            cinfo.focal_len_mm = 8.0; // TODO: don't hard code
            cinfo.pixel_pitch_um = 3.45; // TODO: don't hard code

            ArvPixelFormat pfmt = arv_buffer_get_image_pixel_format(buf);
            if (pfmt == ARV_PIXEL_FORMAT_BAYER_RG_12P)
                cinfo.pixel_fmt = CM_PIXEL_FMT_BAYER_RG12P;
            else if (pfmt == ARV_PIXEL_FORMAT_BAYER_RG_12)
                cinfo.pixel_fmt = CM_PIXEL_FMT_BAYER_RG12;
            else // TODO: handle other pixel formats properly
                cinfo.pixel_fmt = CM_PIXEL_FMT_MONO12;

            size_t imsz;
            const void *raw = (const void *)arv_buffer_get_data(buf, &imsz);
            img.setImage(raw, cinfo);

            // reuse buffer
            arv_stream_push_buffer(this->stream, buf);

            // don't emit if capture was stopped while waiting for frame
            if (this->capturing)
                emit imageCaptured(img);
        } else break;
    }
}
