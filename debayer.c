#include "debayer.h"
#include <assert.h>

void unpack12_16(uint16_t *unpacked, const void *packed12, size_t num_elems, bool scale_up)
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

static inline uint16_t bayer_pixel(const uint16_t *bayer, uint16_t width, uint16_t x, uint16_t y)
{
    return bayer[(y * width) + x];
}

// rgb array is row major, contiguous rgb triplets for each pixel

// classic full resolution debayer
// use local pixel for its channel
// look right for green or red/blue
// look down for the third colour
void debayer22(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height)
{
    // assumes width and height are even
    assert((width & 0x01) == 0);
    assert((height & 0x01) == 0);
    assert(width >= 2);
    assert(height >= 2);

    // bottom right
    // other corners would be handled by main loop
    rgb[(width*height - 1)*3 + 0] = bayer_pixel(bayer, width, width - 2, height - 2);
    rgb[(width*height - 1)*3 + 1] = bayer_pixel(bayer, width, width - 2, height - 1);
    rgb[(width*height - 1)*3 + 2] = bayer_pixel(bayer, width, width - 1, height - 1);

    size_t x, y;

    // right side (GB)
    x = width - 1;
    for (y = 0; y < height - 1;) {
        // green pixel
        rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x - 1, y + 0);
        rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x - 0, y + 0);
        rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x - 0, y + 1);
        y++;

        // blue pixel
        rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x - 1, y + 1);
        rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x - 1, y + 0);
        rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x - 0, y + 0);
        y++;
    }

    // bottom row (GB)
    y = height - 1;
    for (x = 0; x < width - 1;) {
        // green pixel
        rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 0, y - 1);
        rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 0, y - 0);
        rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 1, y - 0);
        x++;

        // blue pixel
        rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 1, y - 1);
        rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 1, y - 0);
        rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 0, y - 0);
        x++;
    }

    // main loop for rest of image
    for (y = 0; y < height - 1;) {
        // RG row
        for (x = 0; x < width - 1;) {
            // red pixel
            rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 0, y + 0);
            rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 1, y + 0);
            rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 1, y + 1);
            x++;

            // green pixel
            rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 1, y + 0);
            rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 0, y + 0);
            rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 0, y + 1);
            x++;
        }
        y++;

        // GB row
        for (x = 0; x < width - 1;) {
            // green pixel
            rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 0, y + 1);
            rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 0, y + 0);
            rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 1, y + 0);
            x++;

            // blue pixel
            rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 1, y + 1);
            rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 1, y + 0);
            rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 0, y + 0);
            x++;
        }
        y++;
    }
}

// averages RGB values from 3x3 square centred around pixel
// centre weighted for green
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

// all these surr_colour_* functions assume (x,y) is at least two pixels away from edge (5x5)
static inline uint32_t surr_colour_rb_same(const uint16_t *bayer, uint16_t width,
        uint16_t x, uint16_t y)
{
    // eight surrounding pixels of same colour within 5x5 square
    uint32_t colour_sum =
        bayer_pixel(bayer, width, x - 2, y - 2) +
        bayer_pixel(bayer, width, x + 0, y - 2) +
        bayer_pixel(bayer, width, x + 2, y - 2) +
        bayer_pixel(bayer, width, x - 2, y + 0) +
        bayer_pixel(bayer, width, x + 2, y + 0) +
        bayer_pixel(bayer, width, x - 2, y + 2) +
        bayer_pixel(bayer, width, x + 0, y + 2) +
        bayer_pixel(bayer, width, x + 2, y + 2);
    return colour_sum / 8;
}

static inline uint32_t surr_colour_rb_opp(const uint16_t *bayer, uint16_t width,
        uint16_t x, uint16_t y)
{
    // four surrounding pixels of opposite colour within 3x3 or 5x5 square
    uint32_t colour_sum =
        bayer_pixel(bayer, width, x - 1, y - 1) +
        bayer_pixel(bayer, width, x + 1, y - 1) +
        bayer_pixel(bayer, width, x - 1, y + 1) +
        bayer_pixel(bayer, width, x + 1, y + 1);
    return colour_sum / 4;
}

static inline uint32_t surr_colour_rb_green(const uint16_t *bayer, uint16_t width,
        uint16_t x, uint16_t y)
{
    // twelve surrounding green pixels within 5x5 square
    uint32_t colour_sum =
        bayer_pixel(bayer, width, x - 1, y - 2) +
        bayer_pixel(bayer, width, x + 1, y - 2) +
        bayer_pixel(bayer, width, x - 2, y - 1) +
        bayer_pixel(bayer, width, x + 0, y - 1) +
        bayer_pixel(bayer, width, x + 2, y - 1) +
        bayer_pixel(bayer, width, x - 1, y + 0) +
        bayer_pixel(bayer, width, x + 1, y + 0) +
        bayer_pixel(bayer, width, x - 2, y + 1) +
        bayer_pixel(bayer, width, x + 0, y + 1) +
        bayer_pixel(bayer, width, x + 2, y + 1) +
        bayer_pixel(bayer, width, x - 1, y + 2) +
        bayer_pixel(bayer, width, x + 1, y + 2);
    return colour_sum / 12;
}

static inline uint32_t surr_colour_g_rowadj(const uint16_t *bayer, uint16_t width,
        uint16_t x, uint16_t y)
{
    // six surrounding pixels of other colour in row
    uint32_t colour_sum =
        bayer_pixel(bayer, width, x - 1, y - 2) +
        bayer_pixel(bayer, width, x + 1, y - 2) +
        bayer_pixel(bayer, width, x - 1, y + 0) +
        bayer_pixel(bayer, width, x + 1, y + 0) +
        bayer_pixel(bayer, width, x - 1, y + 2) +
        bayer_pixel(bayer, width, x + 1, y + 2);
    return colour_sum / 6;
}

static inline uint32_t surr_colour_g_coladj(const uint16_t *bayer, uint16_t width,
        uint16_t x, uint16_t y)
{
    // six surrounding pixels of other colour in column
    uint32_t colour_sum =
        bayer_pixel(bayer, width, x - 2, y - 1) +
        bayer_pixel(bayer, width, x + 0, y - 1) +
        bayer_pixel(bayer, width, x + 2, y - 1) +
        bayer_pixel(bayer, width, x - 2, y + 1) +
        bayer_pixel(bayer, width, x + 0, y + 1) +
        bayer_pixel(bayer, width, x + 2, y + 1);
    return colour_sum / 6;
}

static inline uint32_t surr_colour_g_green(const uint16_t *bayer, uint16_t width,
        uint16_t x, uint16_t y)
{
    // twelve surrounding green pixels
    uint32_t colour_sum =
        bayer_pixel(bayer, width, x - 2, y - 2) +
        bayer_pixel(bayer, width, x + 0, y - 2) +
        bayer_pixel(bayer, width, x + 2, y - 2) +
        bayer_pixel(bayer, width, x - 1, y - 1) +
        bayer_pixel(bayer, width, x + 1, y - 1) +
        bayer_pixel(bayer, width, x - 2, y + 0) +
        bayer_pixel(bayer, width, x + 2, y + 0) +
        bayer_pixel(bayer, width, x - 1, y + 1) +
        bayer_pixel(bayer, width, x + 1, y + 1) +
        bayer_pixel(bayer, width, x - 2, y + 2) +
        bayer_pixel(bayer, width, x + 0, y + 2) +
        bayer_pixel(bayer, width, x + 2, y + 2);
    return colour_sum / 12;
}

// uses local luminance and surrounding chrominance
// slow but sharp and avoids moire
void debayer55(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height)
{
    // TODO: properly handle corners and edges
    // as a lazy wasteful proof of concept, I'm just doing 2x2 first over whole image
    debayer22(bayer, rgb, width, height);

    // main loop for centre
    uint32_t sur_r, sur_g, sur_b;
    for (size_t y = 2; y < height - 2;) {
        // RG row
        for (size_t x = 2; x < width - 2;) {
            // red pixel
            sur_r = surr_colour_rb_same(bayer, width, x, y);
            sur_g = surr_colour_rb_green(bayer, width, x, y);
            sur_b = surr_colour_rb_opp(bayer, width, x, y);
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x] * sur_g / sur_r;
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x] * sur_b / sur_r;
            x++;

            // green pixel
            sur_r = surr_colour_g_rowadj(bayer, width, x, y);
            sur_g = surr_colour_g_green(bayer, width, x, y);
            sur_b = surr_colour_g_coladj(bayer, width, x, y);
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x] * sur_r / sur_g;
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x] * sur_b / sur_g;
            x++;
        }
        y++;

        // GB row
        for (size_t x = 2; x < width - 2;) {
            // green pixel
            sur_r = surr_colour_g_coladj(bayer, width, x, y);
            sur_g = surr_colour_g_green(bayer, width, x, y);
            sur_b = surr_colour_g_rowadj(bayer, width, x, y);
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x] * sur_r / sur_g;
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x] * sur_b / sur_g;
            x++;

            // blue pixel
            sur_r = surr_colour_rb_opp(bayer, width, x, y);
            sur_g = surr_colour_rb_green(bayer, width, x, y);
            sur_b = surr_colour_rb_same(bayer, width, x, y);
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x] * sur_r / sur_b;
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x] * sur_g / sur_b;
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x];
            x++;
        }
        y++;
    }
}

// fast pixel binned 2x2 debayer
// rgb output is half the input width and height
void debayer22_binned(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height)
{
    // assumes width and height are even
    assert((width & 0x01) == 0);
    assert((height & 0x01) == 0);
    assert(width >= 2);
    assert(height >= 2);

    uint16_t width_out = width >> 1;
    uint16_t height_out = height >> 1;

    // RGGB pixel layout for each 2x2 square
    for (size_t y = 0; y < height_out; y++) {
        size_t y_in = y * 2;
        for (size_t x = 0; x < width_out; x++) {
            size_t x_in = x * 2;
            rgb[(width_out*y + x)*3 + 0] = bayer[width*y_in + x_in];
            rgb[(width_out*y + x)*3 + 1] = (bayer[width*y_in + x_in + 1] + bayer[width*(y_in + 1) + x_in]) >> 1;
            rgb[(width_out*y + x)*3 + 2] = bayer[width*(y_in + 1) + x_in + 1];
        }
    }
}
