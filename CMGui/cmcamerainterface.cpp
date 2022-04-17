#include "cmcamerainterface.h"
#include "../cm_camera_helper.h"
#include <QMutexLocker>
#include <cmath>

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
            arv_camera_get_gain_bounds(this->camera, &this->expLimits.gain_min,
                                       &this->expLimits.gain_max, &error);
        if (!error)
            arv_camera_get_exposure_time_bounds(this->camera, &this->expLimits.shutter_min,
                                                &this->expLimits.shutter_max, &error);

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

    if (shutter_us > this->expLimits.shutter_max)
        shutter_us = this->expLimits.shutter_max;
    else if (shutter_us < this->expLimits.shutter_min)
        shutter_us = this->expLimits.shutter_min;

    if (gain_dB > this->expLimits.gain_max)
        gain_dB = this->expLimits.gain_max;
    else if (gain_dB < this->expLimits.gain_min)
        gain_dB = this->expLimits.gain_min;

    GError *error = NULL;
    cinemavi_camera_configure_exposure(this->camera, shutter_us, gain_dB, &error);
    if (error)
        g_clear_object(&this->camera);
    else {
        this->shutter = shutter_us;
        this->gain = gain_dB;
    }
}

void CMCameraInterface::getExposure(double *shutter_us, double *gain_dB)
{
    *shutter_us = this->shutter;
    *gain_dB = this->gain;
}

ExposureLimits & CMCameraInterface::getExposureLimits()
{
    return this->expLimits;
}

void CMCameraInterface::updateExposure(CMExposureMode expMode, double changeFactor)
{
    ExposureParams newExp;
    double maxShutterOk = 250000; // 4 fps minimum
    if (maxShutterOk > this->expLimits.shutter_max)
        maxShutterOk = this->expLimits.shutter_max;

    if (expMode == CMEXP_SHUTTER_PRIORITY) {
        newExp.shutter_us = this->shutter;
        newExp.gain_dB = 20 * log10(pow(10, this->gain / 20) * changeFactor);
        if (newExp.gain_dB > this->expLimits.gain_max)
            newExp.gain_dB = this->expLimits.gain_max;
        else if (newExp.gain_dB < this->expLimits.gain_min)
            newExp.gain_dB = this->expLimits.gain_min;
    } else if (expMode == CMEXP_GAIN_PRIORITY) {
        newExp.shutter_us = this->shutter * changeFactor;
        newExp.gain_dB = this->gain;
        if (newExp.shutter_us > maxShutterOk)
            newExp.shutter_us = maxShutterOk;
        else if (newExp.shutter_us < this->expLimits.shutter_min)
            newExp.shutter_us = this->expLimits.shutter_min;
    } else {
        ExposureLimits limits, targets;
        limits = this->expLimits;
        limits.shutter_max = maxShutterOk;
        targets.shutter_min = 8000;
        targets.shutter_max = 30000;
        targets.gain_min = 5;
        targets.gain_max = 15;

        ExposureParams currentExp = {.shutter_us = this->shutter, .gain_dB = this->gain};
        calculate_exposure(&currentExp, &newExp, &limits, &targets, changeFactor);
    }

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
    if (!this->capturing) return;
    if (!ARV_IS_CAMERA(this->camera)) return;
    this->capturing = false;
    arv_camera_stop_acquisition(this->camera, &error);

    captureThread.quit();
    captureThread.wait();
}

void CMCameraInterface::captureLoop()
{
    while (this->capturing) {
        ArvBuffer *buf = arv_stream_timeout_pop_buffer(this->stream, 300000);
        if (ARV_IS_BUFFER(buf)) {
            CMRawHeader cmrh;
            const void *raw = cinemavi_prepare_header(buf, &cmrh, this->cameraMake.c_str(),
                    this->cameraModel.c_str(), this->shutter, this->gain);

            CMRawImage img;
            img.setImage(raw, cmrh);

            // reuse buffer
            arv_stream_push_buffer(this->stream, buf);

            // don't emit if capture was stopped while waiting for frame
            if (this->capturing)
                emit imageCaptured(img);
        } else break;
    }
}

const std::string & CMCameraInterface::getCameraMake()
{
    return this->cameraMake;
}

const std::string & CMCameraInterface::getCameraModel()
{
    return this->cameraModel;
}

QString CMCameraInterface::getCameraName()
{
    return QString::fromStdString(this->cameraMake + " " + this->cameraModel);
}
