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
void debayer22(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height);
void debayer33(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height);
void debayer55(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height);

// fast pixel binned 2x2 debayer, assumes same pixel layout as above
// rgb output is half input width and height, so it's 3/4 the size of the bayer input buffer
void debayer22_binned(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height);

#ifdef __cplusplus
}
#endif

#endif // DEBAYER_H
