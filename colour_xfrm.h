#ifndef COLOUR_XFRM_H
#define COLOUR_XFRM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* converts rgb 16-bit image to YCbCr floating point image, and adusts white balance,
 * through a single matrix multiplication per pixel
 *
 * warmth:
 *      valid range -1.0 to 1.0
 *      negative values cause red channel to be multiplied by (1 + warmth)
 *      positive values cause blue channel to be multiplied by (1 - warmth)
 * tint:
 *      valid range -1.0 to 1.0
 *      negative values cause red and blue to be multiplied by (1 + tint)
 *      positive values cause green to be multiplied by (1 - tiny)
 */
void colour_xfrm(const uint16_t *rgb, float *ycbcr, uint16_t width, uint16_t height,
        float warmth, float tint);

#ifdef __cplusplus
}
#endif

#endif // COLOUR_XFRM_H
