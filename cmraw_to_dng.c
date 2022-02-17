#include <stdio.h>
#include <stdlib.h>

#include "cm_cli_helper.h"
#include "cmraw.h"

int main (int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: %s [cmr_name] [dng_name]\n", argv[0]);
        return -1;
    }

    if (!endswith(argv[1], ".cmr")) {
        printf("Invalid input extension: %s\n", argv[1]);
        return -1;
    }

    if (!endswith(argv[2], ".dng")) {
        printf("Invalid output extension: %s\n", argv[2]);
        return -1;
    }

    CMRawHeader cmrh;
    void *raw = NULL;

    int status = cmraw_load(&raw, &cmrh, argv[1]);
    if (status != 0) {
        printf("Error %d loading RAW file.\n", status);
    } else {
        cinemavi_generate_dng(raw, &cmrh, argv[2]);
    }

    free(raw);

    return status;
}
