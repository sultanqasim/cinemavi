#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "cm_cli_helper.h"
#include "cmraw.h"
#include "colour_xfrm.h"
#include "debayer.h"

/* Camera Calibration Instructions:
 *
 * Step 1:
 * Display the sRGB primaries (and optionally white) on a high resolution colour accurate
 * display. Take a picture of the display from a distance and focus setting such that individual
 * display pixels are not visible (ie. colour patches look uniform). Record the image as CMRAW.
 * Make sure none of the colour channels clip for accurate saturation. You may need to tweak
 * exposure parameters to guarantee this. The photo should be taken in a dark room with no
 * reflections or haze on the screen. The screen being photographed should be clean and free
 * from smudges or dirt.
 *
 * Suggestion: show lower part of https://www.wide-gamut.com/test on an iPhone
 * If using an iPhone, make sure True Tone and Night Shift are disabled for colour accuracy.
 * The white point of iPhones (and most other decent phones) is roughly D65 in this state.
 *
 * Step 2:
 * Identify X and Y coordinates of pixels in the middle of clean red, green, blue, and optionally
 * white regions. The top left pixel is (0, 0) and the bottom right pixel is (width, height).
 * Supplied coordinates must be at least 3 pixels away from the edges of the image, and at least
 * 3 pixels away from edges of the coloured region.
 *
 * Step 3:
 * Invoke this utility with the CMRAW file and coordinates of the red, green, blue, and (optional)
 * white regions. This will take 7x7 squares centred at the specified coordinates, and find the
 * median RGB values for each of those squares. These are the linear camera RGB values
 * corresponding to the sRGB primaries and D65 white. This utility will generate a camera to
 * sRGB conversion matrix you can use.
 */

typedef struct {
    uint16_t r;
    uint16_t g;
    uint16_t b;
} RGBLinear;

typedef struct {
    uint16_t x;
    uint16_t y;
} PixelCoordinate;

static int u16_cmp(const void *a, const void *b)
{
    return *(uint16_t *)a - *(uint16_t *)b;
}

// assume x and y coordinates are already bounds checked so 7x7 squares stay within image
static uint16_t median_rgb(uint16_t *img_rgb, uint16_t width, uint16_t height,
        uint16_t x, uint16_t y, uint8_t chan)
{
    uint16_t values[49];
    unsigned n = 0;
    (void)height; // unused variable

    if (chan >= 3) return 0;

    for (unsigned i = x - 3; i <= x + 3; i++) {
        for (unsigned j = y - 3; j <= y + 3; j++) {
            values[n++] = img_rgb[3*(j*width + i) + chan];
        }
    }

    // find the median
    qsort(values, 49, sizeof(uint16_t), u16_cmp);
    return values[24];
}

static double colour_intensity(RGBLinear *p)
{
    return sqrt(p->r*p->r + p->g*p->g + p->b*p->b);
}

int main (int argc, char **argv)
{
    int num_primaries;
    if (argc == 8)
        num_primaries = 3;
    else if (argc == 10)
        num_primaries = 4;
    else {
        printf("Usage: %s [cmr_name] [red_x] [red_y] [green_x] [green_y] [blue_x] [blue_y] [white_x] [white_y]\n", argv[0]);
        return -1;
    }

    if (!endswith(argv[1], ".cmr")) {
        printf("Invalid input extension: %s\n", argv[1]);
        return -1;
    }

    CMRawHeader cmrh;
    void *raw = NULL;

    int status = cmraw_load(&raw, &cmrh, argv[1]);
    if (status != 0) {
        printf("Error %d loading RAW file.\n", status);
        return status;
    }

    uint16_t width = cmrh.cinfo.width;
    uint16_t height = cmrh.cinfo.height;

    if (width < 32 || height < 32) {
        printf("Image too small.\n");
        free(raw);
        return -1;
    }

    if (width > CM_MAX_WIDTH || (width & 1) || height > CM_MAX_HEIGHT || (height & 1)) {
        printf("Invalid image size.\n");
        free(raw);
        return -2;
    }

    PixelCoordinate rgbw_coords[4];
    uint16_t x_min = 3;
    uint16_t y_min = 3;
    uint16_t x_max = width - 4;
    uint16_t y_max = height - 4;

    for (int i = 0; i < num_primaries; i++) {
        long x = strtol(argv[2 + i*2], NULL, 0);
        long y = strtol(argv[3 + i*2], NULL, 0);
        if (x < x_min || x > x_max || y < y_min || y > y_max) {
            printf("Invalid coordinates.\n");
            free(raw);
            return -2;
        }
        rgbw_coords[i].x = (uint16_t)x;
        rgbw_coords[i].y = (uint16_t)y;
    }

    uint16_t *rgb12 = (uint16_t *)malloc(3 * width * height * sizeof(uint16_t));
    if (rgb12 == NULL) {
        printf("Out of memory.\n");
        free(raw);
        return -3;
    }

    bool free_bayer = false;
    uint16_t *bayer12 = NULL;
    if (cmrh.cinfo.pixel_fmt == CM_PIXEL_FMT_BAYER_RG12P) {
        bayer12 = (uint16_t *)malloc(width * height * sizeof(uint16_t));
        if (bayer12 == NULL) {
            printf("Out of memory.\n");
            free(raw);
            free(rgb12);
            return -3;
        }
        free_bayer = true;
        unpack12_16(bayer12, raw, width * height, false);
    } else if (cmrh.cinfo.pixel_fmt == CM_PIXEL_FMT_BAYER_RG12) {
        bayer12 = raw;
    } else {
        printf("Invalid pixel format.\n");
        free(raw);
        free(rgb12);
        return -4;
    }

    // Debayer the image, then we can free the raw
    debayer33(bayer12, rgb12, width, height);
    free(raw);
    if (free_bayer) free(bayer12);

    // Now find median RGB values for each of the 7x7 squares centred at specified coordinates
    RGBLinear rgbw_values[4];
    for (int i = 0; i < num_primaries; i++) {
        rgbw_values[i].r = median_rgb(rgb12, width, height, rgbw_coords[i].x, rgbw_coords[i].y, 0);
        rgbw_values[i].g = median_rgb(rgb12, width, height, rgbw_coords[i].x, rgbw_coords[i].y, 1);
        rgbw_values[i].b = median_rgb(rgb12, width, height, rgbw_coords[i].x, rgbw_coords[i].y, 2);
    }

    // Now we're done with the image
    free(rgb12);

    printf("Primaries in Linear Camera Space:\n");
    printf("sRGB    camR   camG   camB\n");
    printf("Red:   %5d  %5d  %5d\n", rgbw_values[0].r, rgbw_values[0].g, rgbw_values[0].b);
    printf("Green: %5d  %5d  %5d\n", rgbw_values[1].r, rgbw_values[1].g, rgbw_values[1].b);
    printf("Blue:  %5d  %5d  %5d\n", rgbw_values[2].r, rgbw_values[2].g, rgbw_values[2].b);
    if (num_primaries > 3)
        printf("White: %5d  %5d  %5d\n", rgbw_values[3].r, rgbw_values[3].g, rgbw_values[3].b);
    printf("\n");

    double intensities[4];
    for (int i = 0; i < num_primaries; i++) {
        intensities[i] = colour_intensity(rgbw_values + i);
    }

    // We will scale camera colours such that sRGB red and blue primary intensities match green
    // The white sample intensity may vary, but at the end we will see how balanced it looks
    // [sRGB_to_cam] * [rgb_primary_vector with intensity matching green primary] = [cam_colour_vector]
    // Each column of sRGB_to_cam is a scaled version of the sRGB primary in camera space
    double inv_green = 1.0 / intensities[1];
    ColourMatrix sRGB_to_cam = {.m={
        rgbw_values[0].r * inv_green, rgbw_values[1].r * inv_green, rgbw_values[2].r * inv_green,
        rgbw_values[0].g * inv_green, rgbw_values[1].g * inv_green, rgbw_values[2].g * inv_green,
        rgbw_values[0].b * inv_green, rgbw_values[1].b * inv_green, rgbw_values[2].b * inv_green
    }};

    // Invert to get cam_to_sRGB
    ColourMatrix cam_to_sRGB;
    colour_matinv33(&cam_to_sRGB, &sRGB_to_cam);

    // See what white looks like using our matrix and rgbw_values[3]
    if (num_primaries > 3) {
        ColourPixel white_cam = {.p={rgbw_values[3].r * inv_green,
            rgbw_values[3].g * inv_green, rgbw_values[3].b * inv_green}};
        ColourPixel white_sRGB;
        ColourMatrix_f cam_to_sRGB_f;
        cmat_d2f(&cam_to_sRGB, &cam_to_sRGB_f);
        pixel_xfrm(&white_cam, &white_sRGB, &cam_to_sRGB_f);

        // We could tweak the matrix to make white_sRGB balanced
        // However, on my iPhone 13, with True Tone disabled, the white has a bluish tinge
        // The RGB primaries on my iPhone seem accurate though, so we can rely on them
        // Thus, we'll just report the computed white value for reference
        printf("sRGB Linear White: %.5f %.5f %.5f\n\n", white_sRGB.p[0], white_sRGB.p[1],
                white_sRGB.p[2]);
    }

    printf("Camera to Linear sRGB Matrix:\n");
    print_mat(&cam_to_sRGB);

    return 0;
}
