#ifndef CMRAW_H
#define CMRAW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    CM_PIXEL_FMT_MONO8,
    CM_PIXEL_FMT_MONO12P,
    CM_PIXEL_FMT_MONO12,
    CM_PIXEL_FMT_MONO16,
    CM_PIXEL_FMT_RGB8,
    CM_PIXEL_FMT_RGB12P,
    CM_PIXEL_FMT_RGB12,
    CM_PIXEL_FMT_RGB16,
    CM_PIXEL_FMT_BAYER_RG8,
    CM_PIXEL_FMT_BAYER_RG12P,
    CM_PIXEL_FMT_BAYER_RG12,
    CM_PIXEL_FMT_BAYER_RG16
} CMPixelFormat;

// by degrees counter-clockwise
typedef enum {
    CM_ORIENTATION_0,
    CM_ORIENTATION_90,
    CM_ORIENTATION_180,
    CM_ORIENTATION_270
} CMOrientation;

#define CM_MAX_WIDTH 32767
#define CM_MAX_HEIGHT 32767

// Header for Cinemavi RAW file
#pragma pack(push, 1)
typedef struct {
    uint32_t magic;     // shoudl be little endian 0x69564D43 ie. CMVi
    uint8_t pixel_fmt;  // CMPixelFormat enum member
    uint8_t orientation;// CMOrientation enum member
    uint8_t reserved1;
    uint8_t reserved2;
    uint64_t ts_epoch;  // unix time
    uint16_t width;
    uint16_t height;
    float shutter_us;
    float gain_dB;
    float focal_len;
    float pixel_pitch_um;
    char reserved3[32];
    char camera_make[32];
    char camer_model[32];
    char capture_software[32];
} CMCaptureInfo;
#pragma pack(pop)

// Zero the struct, set the magic and timestamp
void cm_capture_info_init(CMCaptureInfo *cinfo);

int cmraw_save(const void *raw, const CMCaptureInfo *cinfo, const char *fname);

// sets the raw pointer, caller must free (with C stdlib free) when done
int cmraw_load(void **raw, CMCaptureInfo *cinfo, const char *fname);

#ifdef __cplusplus
}
#endif

#endif // CMRAW_H
