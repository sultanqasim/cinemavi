#include "debayer.h"
#include <assert.h>

void unpack12_16(const void *packed12, uint16_t *unpacked, size_t num_elems, bool scale_up)
{
    const uint8_t *packed = (const uint8_t *)packed12;
    size_t n = 0;
    unsigned odd_elems = num_elems & 1;

    if (!scale_up) {
        while (n < num_elems - odd_elems) {
            size_t r = (n >> 1) * 3;
            unpacked[n++] = packed[r] | ((packed[r+1] & 0x0F) << 8);
            unpacked[n++] = (packed[r+1] >> 4) | (packed[r+2] << 4);
        }

        if (odd_elems) {
            size_t r = (n >> 1) * 3;
            unpacked[n++] = packed[r] | ((packed[r+1] & 0x0F) << 8);
        }
    } else {
        // bitshift unpacked value up by 4 bits to occupy MSBs of 16 bit integer
        while (n < num_elems - odd_elems) {
            size_t r = (n >> 1) * 3;
            unpacked[n++] = (packed[r] << 4) | ((packed[r+1] & 0x0F) << 12);
            unpacked[n++] = (packed[r+1] & 0xF0) | (packed[r+2] << 8);
        }

        if (odd_elems) {
            size_t r = (n >> 1) * 3;
            unpacked[n++] = (packed[r] << 4) | ((packed[r+1] & 0x0F) << 12);
        }
    }
}

// rgb array is row major, contiguous rgb triplets for each pixel
void debayer33(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height)
{
    // assumes width and height are even
    assert((width & 0x01) == 0);
    assert((height & 0x01) == 0);
    assert(width >= 2);
    assert(height >= 2);

    // four corners (RGGB)
    rgb[0] = bayer[0];
    rgb[1] = (bayer[1] + bayer[width]) >> 1;
    rgb[2] = bayer[width + 1];

    rgb[(width - 1)*3 + 0] = bayer[width - 2];
    rgb[(width - 1)*3 + 1] = (bayer[width - 1]*3 + bayer[width*2 - 2]) >> 2;
    rgb[(width - 1)*3 + 2] = bayer[width*2 - 1];

    rgb[width*(height - 1)*3 + 0] = bayer[width*(height - 2)];
    rgb[width*(height - 1)*3 + 1] = (bayer[width*(height - 1)]*3 + bayer[width*(height-2) + 1]) >> 2;
    rgb[width*(height - 1)*3 + 2] = bayer[width*(height - 1) + 1];

    rgb[(width*height - 1)*3 + 0] = bayer[width*(height - 1) - 2];
    rgb[(width*height - 1)*3 + 1] = (bayer[width*height - 2] + bayer[width*(height-1) - 1]) >> 1;
    rgb[(width*height - 1)*3 + 2] = bayer[width*height - 1];

    // top row (GR)
    for (size_t x = 1; x < width - 1;) {
        // green pixel
        rgb[x*3 + 0] = (bayer[x-1] + bayer[x+1]) >> 1;
        rgb[x*3 + 1] = (bayer[x]*2 + bayer[width+x-1] + bayer[width+x+1]) >> 2;
        rgb[x*3 + 2] = bayer[width + x];
        x++;

        // red pixel
        rgb[x*3 + 0] = bayer[x];
        rgb[x*3 + 1] = (bayer[width+x]*2 + bayer[x-1]*3 + bayer[x+1]*3) >> 3;
        rgb[x*3 + 2] = (bayer[width+x-1] + bayer[width+x+1]) >> 1;
        x++;
    }

    // left side (GR)
    for (size_t y = 1; y < height - 1;) {
        // green pixel
        rgb[width*y*3 + 0] = (bayer[width*(y-1)] + bayer[width*(y+1)]) >> 1;
        rgb[width*y*3 + 1] = (bayer[width*y]*2 + bayer[width*(y-1) + 1] + bayer[width*(y+1) + 1]) >> 2;
        rgb[width*y*3 + 2] = bayer[width*y + 1];
        y++;

        // red pixel
        rgb[width*y*3 + 0] = bayer[width*y];
        rgb[width*y*3 + 1] = (bayer[width*y + 1]*2 + bayer[width*(y-1)]*3 + bayer[width*(y+1)]*3) >> 3;
        rgb[width*y*3 + 2] = (bayer[width*(y-1) + 1] + bayer[width*(y+1) + 1]) >> 1;
        y++;
    }

    // right side (BG)
    for (size_t y = 1; y < height - 1;) {
        // blue pixel
        rgb[(width*(y+1) - 1)*3 + 0] = (bayer[width*y - 2] + bayer[width*(y+2) - 2]) >> 1;
        rgb[(width*(y+1) - 1)*3 + 1] = (bayer[width*(y+1) - 2]*2 + bayer[width*(y+2) - 1]*3 + bayer[width*y - 1]*3) >> 3;
        rgb[(width*(y+1) - 1)*3 + 2] = bayer[width*(y+1) - 1];
        y++;

        // green pixel
        rgb[(width*(y+1) - 1)*3 + 0] = bayer[width*(y+1) - 2];
        rgb[(width*(y+1) - 1)*3 + 1] = (bayer[width*(y+1) - 1]*2 + bayer[width*(y+2) - 2] + bayer[width*y - 2]) >> 2;
        rgb[(width*(y+1) - 1)*3 + 2] = (bayer[width*y - 1] + bayer[width*(y+2) - 1]) >> 1;
        y++;
    }

    // bottom row (BG)
    for (size_t x = 1; x < width - 1;) {
        // blue pixel
        rgb[(width*(height-1) + x)*3 + 0] = (bayer[width*(height-2) + x - 1] + bayer[width*(height-2) + x + 1]) >> 1;
        rgb[(width*(height-1) + x)*3 + 1] = (bayer[width*(height-2) + x]*2 + bayer[width*(height-1) + x - 1]*3 +
                bayer[width*(height-1) + x + 1]*3) >> 3;
        rgb[(width*(height-1) + x)*3 + 2] = bayer[width*(height-1) + x];
        x++;

        // green pixel
        rgb[(width*(height-1) + x)*3 + 0] = bayer[width*(height-2) + x];
        rgb[(width*(height-1) + x)*3 + 1] = (bayer[width*(height-1) + x]*2 + bayer[width*(height-2) + x - 1] +
                bayer[width*(height-2) + x + 1]) >> 2;
        rgb[(width*(height-1) + x)*3 + 2] = (bayer[width*(height-1) + x - 1] + bayer[width*(height-1) + x + 1]) >> 1;
        x++;
    }

    // main loop for centre
    for (size_t y = 1; y < height - 1;) {
        // BG row
        for (size_t x = 1; x < width - 1;) {
            // blue pixel
            rgb[(width*y + x)*3 + 0] = (bayer[width*(y-1) + x - 1] + bayer[width*(y-1) + x + 1] +
                    bayer[width*(y+1) + x - 1] + bayer[width*(y+1) + x + 1]) >> 2;
            rgb[(width*y + x)*3 + 1] = (bayer[width*(y-1) + x] + bayer[width*y + x - 1] +
                    bayer[width*y + x + 1] + bayer[width*(y+1) + x]) >> 2;
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x];
            x++;

            // green pixel
            rgb[(width*y + x)*3 + 0] = (bayer[width*(y-1) + x] + bayer[width*(y+1) + x]) >> 1;
            rgb[(width*y + x)*3 + 1] = (bayer[width*y + x]*4 + bayer[width*(y-1) + x - 1] + bayer[width*(y-1) + x + 1] +
                    bayer[width*(y+1) + x - 1] + bayer[width*(y+1) + x + 1]) >> 3;
            rgb[(width*y + x)*3 + 2] = (bayer[width*y + x - 1] + bayer[width*y + x + 1]) >> 1;
            x++;
        }
        y++;

        // GR row
        for (size_t x = 1; x < width - 1;) {
            // green pixel
            rgb[(width*y + x)*3 + 0] = (bayer[width*y + x - 1] + bayer[width*y + x + 1]) >> 1;
            rgb[(width*y + x)*3 + 1] = (bayer[width*y + x]*4 + bayer[width*(y-1) + x - 1] + bayer[width*(y-1) + x + 1] +
                    bayer[width*(y+1) + x - 1] + bayer[width*(y+1) + x + 1]) >> 3;
            rgb[(width*y + x)*3 + 2] = (bayer[width*(y-1) + x] + bayer[width*(y+1) + x]) >> 1;
            x++;

            // red pixel
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 1] = (bayer[width*(y-1) + x] + bayer[width*y + x - 1] +
                    bayer[width*y + x + 1] + bayer[width*(y+1) + x]) >> 2;
            rgb[(width*y + x)*3 + 2] = (bayer[width*(y-1) + x - 1] + bayer[width*(y-1) + x + 1] +
                    bayer[width*(y+1) + x - 1] + bayer[width*(y+1) + x + 1]) >> 2;
            x++;
        }
        y++;
    }
}

// TODO
void debayer55(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height);
