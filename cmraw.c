#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "cmraw.h"

#define CM_MAGIC 0x69564D43

// Zero the struct, set the magic and timestamp
void cm_capture_info_init(CMCaptureInfo *cinfo)
{
    memset(cinfo, 0, sizeof(CMCaptureInfo));
    cinfo->magic = CM_MAGIC;
    cinfo->ts_epoch = time(NULL);
}

static size_t get_raw_len(CMPixelFormat fmt, uint16_t width, uint16_t height)
{
    if (fmt == CM_PIXEL_FMT_MONO8 ||
            fmt == CM_PIXEL_FMT_BAYER_RG8)
        return width * height;
    else if (fmt == CM_PIXEL_FMT_MONO12P ||
            fmt == CM_PIXEL_FMT_BAYER_RG12P)
        return (width * height * 3) >> 1;
    else if (fmt == CM_PIXEL_FMT_MONO12 ||
            fmt == CM_PIXEL_FMT_MONO16 ||
            fmt == CM_PIXEL_FMT_BAYER_RG12 ||
            fmt == CM_PIXEL_FMT_BAYER_RG16)
        return width * height * 2;
    else if (fmt == CM_PIXEL_FMT_RGB8)
        return width * height * 3;
    else if (fmt == CM_PIXEL_FMT_RGB12P)
        return (width * height * 9ULL) >> 1;
    else if (fmt == CM_PIXEL_FMT_RGB12 ||
            fmt == CM_PIXEL_FMT_RGB16)
        return width * height * 6ULL;
    else
        return 0;
}

int cmraw_save(const void *raw, const CMCaptureInfo *cinfo, const char *fname)
{
    if (raw == NULL || cinfo == NULL || fname == NULL)
        return -EINVAL;

    if (cinfo->magic != CM_MAGIC)
        return -EINVAL;

    if (cinfo->width > CM_MAX_WIDTH | cinfo->height > CM_MAX_HEIGHT)
        return -EOVERFLOW;

    size_t raw_len = get_raw_len(cinfo->pixel_fmt, cinfo->width, cinfo->height);
    if (raw_len == 0)
        return -EINVAL;

    FILE *f = fopen(fname, "wb");
    if (f == NULL)
        return -errno;

    int status = 0;
    if (fwrite(cinfo, sizeof(CMCaptureInfo), 1, f) != 1)
        status = -EIO;

    if (!status && fwrite(raw, raw_len, 1, f) != 1)
        status = -EIO;

    fclose(f);

    return status;
}

// sets the raw pointer, caller must free (with C stdlib free) when done
int cmraw_load(void **raw, CMCaptureInfo *cinfo, const char *fname)
{
    if (raw == NULL || cinfo == NULL || fname == NULL)
        return -EINVAL;

    FILE *f = fopen(fname, "rb");
    if (f == NULL)
        return -errno;

    int status = 0;
    if (fread(cinfo, sizeof(CMCaptureInfo), 1, f) != 1)
        status = -EIO;

    size_t raw_len = 0;

    if (!status) {
        if (cinfo->magic != CM_MAGIC)
            status = -EINVAL;
        else if (cinfo->width > CM_MAX_WIDTH | cinfo->height > CM_MAX_HEIGHT)
            status = -EOVERFLOW;
        else
            raw_len = get_raw_len(cinfo->pixel_fmt, cinfo->width, cinfo->height);
    }

    if (!status && raw_len == 0)
        status = -EINVAL;

    if (!status) {
        *raw = malloc(raw_len);
        if (*raw == NULL)
            status = -ENOMEM;
    }

    if (!status && fread(*raw, 1, raw_len, f) != raw_len)
        status = -EIO;

    fclose(f);

    return status;
}
