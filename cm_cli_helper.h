#ifndef CM_CLI_HELPER_H
#define CM_CLI_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "cmraw.h"
#include "pipeline.h"

extern const ImagePipelineParams default_pipeline_params;

void cinemavi_generate_dng(const void *raw, const CMRawHeader *cmrh,
        const char *fname);

void cinemavi_generate_tiff(const void *raw, const CMRawHeader *cmrh,
        const char *fname);

void cinemavi_generate_cmr(const void *raw, const CMRawHeader *cmrh,
        const char *fname);

bool endswith(const char *s, const char *suffix);

#ifdef __cplusplus
}
#endif

#endif // CM_CLI_HELPER_H
