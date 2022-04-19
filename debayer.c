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
    for (y = 0; y < height - 1; y++) {
        if ((y & 1) == 0) {
            // green pixel
            rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x - 1, y + 0);
            rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x - 0, y + 0);
            rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x - 0, y + 1);
        } else {
            // blue pixel
            rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x - 1, y + 1);
            rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x - 1, y + 0);
            rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x - 0, y + 0);
        }
    }

    // bottom row (GB)
    y = height - 1;
    for (x = 0; x < width - 1; x++) {
        if ((x & 1) == 0) {
            // green pixel
            rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 0, y - 1);
            rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 0, y - 0);
            rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 1, y - 0);
        } else {
            // blue pixel
            rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 1, y - 1);
            rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 1, y - 0);
            rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 0, y - 0);
        }
    }

    // main loop for rest of image
    for (y = 0; y < height - 1; y++) {
        if ((y & 1) == 0) {
            // RG row
            for (x = 0; x < width - 1; x++) {
                if ((x & 1) == 0) {
                    // red pixel
                    rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 0, y + 0);
                    rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 1, y + 0);
                    rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 1, y + 1);
                } else {
                    // green pixel
                    rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 1, y + 0);
                    rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 0, y + 0);
                    rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 0, y + 1);
                }
            }
        } else {
            // GB row
            for (x = 0; x < width - 1; x++) {
                if ((x & 1) == 0) {
                    // green pixel
                    rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 0, y + 1);
                    rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 0, y + 0);
                    rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 1, y + 0);
                } else {
                    // blue pixel
                    rgb[(width*y + x)*3 + 0] = bayer_pixel(bayer, width, x + 1, y + 1);
                    rgb[(width*y + x)*3 + 1] = bayer_pixel(bayer, width, x + 1, y + 0);
                    rgb[(width*y + x)*3 + 2] = bayer_pixel(bayer, width, x + 0, y + 0);
                }
            }
        }
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
    rgb[(width - 1)*3 + 1] = bayer[width - 1];
    rgb[(width - 1)*3 + 2] = bayer[width*2 - 1];

    rgb[width*(height - 1)*3 + 0] = bayer[width*(height - 2)];
    rgb[width*(height - 1)*3 + 1] = bayer[width*(height - 1)];
    rgb[width*(height - 1)*3 + 2] = bayer[width*(height - 1) + 1];

    rgb[(width*height - 1)*3 + 0] = bayer[width*(height - 1) - 2];
    rgb[(width*height - 1)*3 + 1] = (bayer[width*height - 2] + bayer[width*(height-1) - 1]) >> 1;
    rgb[(width*height - 1)*3 + 2] = bayer[width*height - 1];

    // top row (GR)
    for (size_t x = 1; x < width - 1;) {
        // green pixel
        rgb[x*3 + 0] = (bayer[x-1] + bayer[x+1]) >> 1;
        rgb[x*3 + 1] = bayer[x];
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
        rgb[width*y*3 + 1] = bayer[width*y];
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
        rgb[(width*(y+1) - 1)*3 + 1] = bayer[width*(y+1) - 1];
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
        rgb[(width*(height-1) + x)*3 + 1] = bayer[width*(height-1) + x];
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
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 2] = (bayer[width*y + x - 1] + bayer[width*y + x + 1]) >> 1;
            x++;
        }
        y++;

        // GR row
        for (size_t x = 1; x < width - 1;) {
            // green pixel
            rgb[(width*y + x)*3 + 0] = (bayer[width*y + x - 1] + bayer[width*y + x + 1]) >> 1;
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x];
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

// all these surr_colour_edge_* functions perform bounds checking on x and y to ensure no OOB reads at edges
// they still expect 0 <= x < width and 0 <= y < height to avoid SIGFPE

static inline uint32_t add_pixel_edge(const uint16_t *bayer, uint16_t width, uint16_t height,
        uint32_t *sum, uint16_t x, uint16_t y)
{
    if (x < width && y < height) {
        *sum += bayer_pixel(bayer, width, x, y);
        return 1;
    } else {
        return 0;
    }
}

static inline uint32_t surr_colour_edge_rb_same(const uint16_t *bayer, uint16_t width, uint16_t height,
        uint16_t x, uint16_t y)
{
    // up to eight surrounding pixels of same colour within 5x5 square
    uint32_t num_pixels = 0;
    uint32_t colour_sum = 0;

    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 2, y - 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 0, y - 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 2, y - 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 2, y + 0);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 2, y + 0);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 2, y + 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 0, y + 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 2, y + 2);

    return colour_sum / num_pixels;
}

static inline uint32_t surr_colour_edge_rb_opp(const uint16_t *bayer, uint16_t width, uint16_t height,
        uint16_t x, uint16_t y)
{
    // up to four surrounding pixels of opposite colour within 3x3 or 5x5 square
    uint32_t num_pixels = 0;
    uint32_t colour_sum = 0;

    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 1, y - 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 1, y - 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 1, y + 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 1, y + 1);

    return colour_sum / num_pixels;
}

static inline uint32_t surr_colour_edge_rb_green(const uint16_t *bayer, uint16_t width, uint16_t height,
        uint16_t x, uint16_t y)
{
    // up to twelve surrounding green pixels within 5x5 square
    uint32_t num_pixels = 0;
    uint32_t colour_sum = 0;

    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 1, y - 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 1, y - 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 2, y - 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 0, y - 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 2, y - 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 1, y + 0);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 1, y + 0);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 2, y + 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 0, y + 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 2, y + 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 1, y + 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 1, y + 2);

    return colour_sum / num_pixels;
}

static inline uint32_t surr_colour_edge_g_rowadj(const uint16_t *bayer, uint16_t width, uint16_t height,
        uint16_t x, uint16_t y)
{
    // up to six surrounding pixels of other colour in row
    uint32_t num_pixels = 0;
    uint32_t colour_sum = 0;

    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 1, y - 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 1, y - 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 1, y + 0);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 1, y + 0);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 1, y + 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 1, y + 2);

    return colour_sum / num_pixels;
}

static inline uint32_t surr_colour_edge_g_coladj(const uint16_t *bayer, uint16_t width, uint16_t height,
        uint16_t x, uint16_t y)
{
    // up to six surrounding pixels of other colour in column
    uint32_t num_pixels = 0;
    uint32_t colour_sum = 0;

    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 2, y - 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 0, y - 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 2, y - 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 2, y + 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 0, y + 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 2, y + 1);

    return colour_sum / num_pixels;
}

static inline uint32_t surr_colour_edge_g_green(const uint16_t *bayer, uint16_t width, uint16_t height,
        uint16_t x, uint16_t y)
{
    // up to twelve surrounding green pixels
    uint32_t num_pixels = 0;
    uint32_t colour_sum = 0;

    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 2, y - 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 0, y - 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 2, y - 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 1, y - 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 1, y - 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 2, y + 0);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 2, y + 0);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 1, y + 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 1, y + 1);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x - 2, y + 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 0, y + 2);
    num_pixels += add_pixel_edge(bayer, width, height, &colour_sum, x + 2, y + 2);

    return colour_sum / num_pixels;
}

static void edge_pixel_debayer55(const uint16_t *bayer, uint16_t *rgb,
        uint16_t width, uint16_t height, uint16_t x, uint16_t y)
{
    uint32_t sur_r, sur_g, sur_b;

    if ((y & 1) == 0) {
        // RG row
        if ((x & 1) == 0) {
            // red pixel
            sur_r = surr_colour_edge_rb_same(bayer, width, height, x, y);
            sur_g = surr_colour_edge_rb_green(bayer, width, height, x, y);
            sur_b = surr_colour_edge_rb_opp(bayer, width, height, x, y);
            if (sur_r == 0) sur_r = 1;
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x] * sur_g / sur_r;
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x] * sur_b / sur_r;
        } else {
            // green pixel
            sur_r = surr_colour_edge_g_rowadj(bayer, width, height, x, y);
            sur_g = surr_colour_edge_g_green(bayer, width, height, x, y);
            sur_b = surr_colour_edge_g_coladj(bayer, width, height, x, y);
            if (sur_g == 0) sur_g = 1;
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x] * sur_r / sur_g;
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x] * sur_b / sur_g;
        }
    } else {
        // GB row
        if ((x & 1) == 0) {
            // green pixel
            sur_r = surr_colour_edge_g_coladj(bayer, width, height, x, y);
            sur_g = surr_colour_edge_g_green(bayer, width, height, x, y);
            sur_b = surr_colour_edge_g_rowadj(bayer, width, height, x, y);
            if (sur_g == 0) sur_g = 1;
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x] * sur_r / sur_g;
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x] * sur_b / sur_g;
        } else {
            // blue pixel
            sur_r = surr_colour_edge_rb_opp(bayer, width, height, x, y);
            sur_g = surr_colour_edge_rb_green(bayer, width, height, x, y);
            sur_b = surr_colour_edge_rb_same(bayer, width, height, x, y);
            if (sur_b == 0) sur_b = 1;
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x] * sur_r / sur_b;
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x] * sur_g / sur_b;
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x];
        }
    }
}

// uses local luminance and surrounding chrominance
// slow but sharp and avoids moire
void debayer55(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height)
{
    // corners and edges
    for (size_t y = 0; y < 2; y++)
        for (size_t x = 0; x < width; x++)
            edge_pixel_debayer55(bayer, rgb, width, height, x, y);
    for (size_t y = height - 2; y < height; y++)
        for (size_t x = 0; x < width; x++)
            edge_pixel_debayer55(bayer, rgb, width, height, x, y);
    for (size_t x = 0; x < 2; x++)
        for (size_t y = 2; y < height - 2; y++)
            edge_pixel_debayer55(bayer, rgb, width, height, x, y);
    for (size_t x = width - 2; x < width; x++)
        for (size_t y = 2; y < height - 2; y++)
            edge_pixel_debayer55(bayer, rgb, width, height, x, y);

    // main loop for centre
    uint32_t sur_r, sur_g, sur_b;
    for (size_t y = 2; y < height - 2;) {
        // RG row
        for (size_t x = 2; x < width - 2;) {
            // red pixel
            sur_r = surr_colour_rb_same(bayer, width, x, y);
            sur_g = surr_colour_rb_green(bayer, width, x, y);
            sur_b = surr_colour_rb_opp(bayer, width, x, y);
            if (sur_r == 0) sur_r = 1;
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x] * sur_g / sur_r;
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x] * sur_b / sur_r;
            x++;

            // green pixel
            sur_r = surr_colour_g_rowadj(bayer, width, x, y);
            sur_g = surr_colour_g_green(bayer, width, x, y);
            sur_b = surr_colour_g_coladj(bayer, width, x, y);
            if (sur_g == 0) sur_g = 1;
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
            if (sur_g == 0) sur_g = 1;
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x] * sur_r / sur_g;
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x] * sur_b / sur_g;
            x++;

            // blue pixel
            sur_r = surr_colour_rb_opp(bayer, width, x, y);
            sur_g = surr_colour_rb_green(bayer, width, x, y);
            sur_b = surr_colour_rb_same(bayer, width, x, y);
            if (sur_b == 0) sur_b = 1;
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x] * sur_r / sur_b;
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x] * sur_g / sur_b;
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x];
            x++;
        }
        y++;
    }
}

static inline uint16_t absdiff(uint16_t a, uint16_t b)
{
    return a > b ? a - b : b - a;
}

// mask bits are for gradients 45 degrees going clockwise
static uint8_t vng_mask(const uint16_t *bayer, uint16_t width, uint16_t height, uint16_t x, uint16_t y)
{
    uint16_t gradients[8];
    uint16_t local = bayer_pixel(bayer, width, x, y);
    uint16_t grad_thresh = local / 4;
    uint8_t edge_mask = 0xFF;

    if (x < 2 || x >= width - 2 || y < 2 || y >= height - 2) {
        // slower edge checking path
        if (x < 2)
            edge_mask &= ~0x38;
        if (x >= width - 2)
            edge_mask &= ~0x83;
        if (y < 2)
            edge_mask &= ~0x0E;
        if (y >= height - 2)
            edge_mask &= ~0xE0;

        gradients[0] = (edge_mask & 0x01) ? absdiff(local, bayer_pixel(bayer, width, x + 2, y + 0)) : 0xFFFF;
        gradients[1] = (edge_mask & 0x02) ? absdiff(local, bayer_pixel(bayer, width, x + 2, y - 2)) : 0xFFFF;
        gradients[2] = (edge_mask & 0x04) ? absdiff(local, bayer_pixel(bayer, width, x + 0, y - 2)) : 0xFFFF;
        gradients[3] = (edge_mask & 0x08) ? absdiff(local, bayer_pixel(bayer, width, x - 2, y - 2)) : 0xFFFF;
        gradients[4] = (edge_mask & 0x10) ? absdiff(local, bayer_pixel(bayer, width, x - 2, y + 0)) : 0xFFFF;
        gradients[5] = (edge_mask & 0x20) ? absdiff(local, bayer_pixel(bayer, width, x - 2, y + 2)) : 0xFFFF;
        gradients[6] = (edge_mask & 0x40) ? absdiff(local, bayer_pixel(bayer, width, x + 0, y + 2)) : 0xFFFF;
        gradients[7] = (edge_mask & 0x80) ? absdiff(local, bayer_pixel(bayer, width, x + 2, y + 2)) : 0xFFFF;
    } else {
        // faster centre path
        gradients[0] = absdiff(local, bayer_pixel(bayer, width, x + 2, y + 0));
        gradients[1] = absdiff(local, bayer_pixel(bayer, width, x + 2, y - 2));
        gradients[2] = absdiff(local, bayer_pixel(bayer, width, x + 0, y - 2));
        gradients[3] = absdiff(local, bayer_pixel(bayer, width, x - 2, y - 2));
        gradients[4] = absdiff(local, bayer_pixel(bayer, width, x - 2, y + 0));
        gradients[5] = absdiff(local, bayer_pixel(bayer, width, x - 2, y + 2));
        gradients[6] = absdiff(local, bayer_pixel(bayer, width, x + 0, y + 2));
        gradients[7] = absdiff(local, bayer_pixel(bayer, width, x + 2, y + 2));
    }

    uint8_t mask = 0x00;
    for (int i = 0; i < 8; i++) {
        if (gradients[i] < grad_thresh)
            mask |= 1 << i;
    }

    // we need at least one vertical, one horizontal, and one diagonal axis
    // in the mask to calculate the other two colours within a 3x3 square
    if (!(mask & 0x11))
        mask |= edge_mask & 0x11;
    if (!(mask & 0x44))
        mask |= edge_mask & 0x44;
    if (!(mask & 0xAA))
        mask |= edge_mask & 0xAA;

    return mask;
}

static inline uint32_t surr_colour_rb_opp_vng(const uint16_t *bayer, uint16_t width,
        uint16_t x, uint16_t y, uint8_t mask)
{
    // four surrounding pixels of opposite colour within 3x3
    uint32_t num_pixels = 0;
    uint32_t colour_sum = 0;

    if (mask & 0x08) {
        colour_sum += bayer_pixel(bayer, width, x - 1, y - 1);
        num_pixels++;
    }
    if (mask & 0x02) {
        colour_sum += bayer_pixel(bayer, width, x + 1, y - 1);
        num_pixels++;
    }
    if (mask & 0x20) {
        colour_sum += bayer_pixel(bayer, width, x - 1, y + 1);
        num_pixels++;
    }
    if (mask & 0x80) {
        colour_sum += bayer_pixel(bayer, width, x + 1, y + 1);
        num_pixels++;
    }

    return colour_sum / num_pixels;
}

static inline uint32_t surr_colour_rb_green_vng(const uint16_t *bayer, uint16_t width,
        uint16_t x, uint16_t y, uint8_t mask)
{
    // four surrounding green pixels within 3x3 square
    uint32_t num_pixels = 0;
    uint32_t colour_sum = 0;

    if (mask & 0x01) {
        colour_sum += bayer_pixel(bayer, width, x + 1, y + 0);
        num_pixels++;
    }
    if (mask & 0x04) {
        colour_sum += bayer_pixel(bayer, width, x + 0, y - 1);
        num_pixels++;
    }
    if (mask & 0x10) {
        colour_sum += bayer_pixel(bayer, width, x - 1, y + 0);
        num_pixels++;
    }
    if (mask & 0x40) {
        colour_sum += bayer_pixel(bayer, width, x + 0, y + 1);
        num_pixels++;
    }

    return colour_sum / num_pixels;
}

static inline uint32_t surr_colour_g_rowadj_vng(const uint16_t *bayer, uint16_t width,
        uint16_t x, uint16_t y, uint8_t mask)
{
    // two surrounding pixels of other colour in row within 3x3
    uint32_t num_pixels = 0;
    uint32_t colour_sum = 0;

    if (mask & 0x10) {
        colour_sum += bayer_pixel(bayer, width, x - 1, y + 0);
        num_pixels++;
    }
    if (mask & 0x01) {
        colour_sum += bayer_pixel(bayer, width, x + 1, y + 0);
        num_pixels++;
    }

    return colour_sum / num_pixels;
}

static inline uint32_t surr_colour_g_coladj_vng(const uint16_t *bayer, uint16_t width,
        uint16_t x, uint16_t y, uint8_t mask)
{
    // six surrounding pixels of other colour in column within 3x3
    uint32_t num_pixels = 0;
    uint32_t colour_sum = 0;

    if (mask & 0x04) {
        colour_sum += bayer_pixel(bayer, width, x + 0, y - 1);
        num_pixels++;
    }
    if (mask & 0x40) {
        colour_sum += bayer_pixel(bayer, width, x + 0, y + 1);
        num_pixels++;
    }

    return colour_sum / num_pixels;
}

// variable number of gradients algorithm
void debayer_vng(const uint16_t *bayer, uint16_t *rgb, uint16_t width, uint16_t height)
{
    uint32_t sur_r, sur_g, sur_b;
    uint16_t mask;

    for (size_t y = 0; y < height;) {
        // RG row
        for (size_t x = 0; x < width;) {
            // red pixel
            mask = vng_mask(bayer, width, height, x, y);
            sur_g = surr_colour_rb_green_vng(bayer, width, x, y, mask);
            sur_b = surr_colour_rb_opp_vng(bayer, width, x, y, mask);
            rgb[(width*y + x)*3 + 0] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 1] = sur_g;
            rgb[(width*y + x)*3 + 2] = sur_b;
            x++;

            // green pixel
            mask = vng_mask(bayer, width, height, x, y);
            sur_r = surr_colour_g_rowadj_vng(bayer, width, x, y, mask);
            sur_b = surr_colour_g_coladj_vng(bayer, width, x, y, mask);
            rgb[(width*y + x)*3 + 0] = sur_r;
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 2] = sur_b;
            x++;
        }
        y++;

        // GB row
        for (size_t x = 0; x < width;) {
            // green pixel
            mask = vng_mask(bayer, width, height, x, y);
            sur_r = surr_colour_g_coladj_vng(bayer, width, x, y, mask);
            sur_b = surr_colour_g_rowadj_vng(bayer, width, x, y, mask);
            rgb[(width*y + x)*3 + 0] = sur_r;
            rgb[(width*y + x)*3 + 1] = bayer[width*y + x];
            rgb[(width*y + x)*3 + 2] = sur_b;
            x++;

            // blue pixel
            mask = vng_mask(bayer, width, height, x, y);
            sur_r = surr_colour_rb_opp_vng(bayer, width, x, y, mask);
            sur_g = surr_colour_rb_green_vng(bayer, width, x, y, mask);
            rgb[(width*y + x)*3 + 0] = sur_r;
            rgb[(width*y + x)*3 + 1] = sur_g;
            rgb[(width*y + x)*3 + 2] = bayer[width*y + x];
            x++;
        }
        y++;
    }
}

// fast pixel binned (superpixel) 2x2 debayer
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
