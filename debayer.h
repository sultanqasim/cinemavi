#ifndef DEBAYER_H
#define DEBAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void unpack12_16(uint16_t *unpacked, const void *packed12, size_t num_elems, bool scale_up);

// assume RGGB pixel layout for each 2x2 square starting at top, and column major image layout
// output buffer (rgb) should be 3x size of input buffer (bayer)
void debayer33(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height);
void debayer55(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height);

#ifdef __cplusplus
}
#endif

#endif // DEBAYER_H
