#include <math.h>
#include <assert.h>
#include "gamma.h"

// multiply gamma corrector by this to have deep blacks
static inline double black_crush(double x)
{
    if (x < 0.05)
        return 1.0 - pow(0.01, 50*x);
    else
        return 1.0;
}

// generate lut for gamma encoding a linear space image
// bit depth should be between 8 and 16
// length of lut should be 1 << bit_depth
void gamma_gen_lut(uint8_t *lut, uint8_t bit_depth)
{
    double G = 8.0 / bit_depth;
    double i_scale = 1.0 / (1 << bit_depth);

    assert(bit_depth >= 8);
    assert(bit_depth <= 16);

    for (int i = 0; i < 1 << bit_depth; i++) {
        double x = i * i_scale;
        lut[i] = 255.999 * pow(x, G) * black_crush(x);
    }
}

// apply cubic base curve before gamma encoding
// shadow must be in range [0.001, 2.999]
void gamma_gen_lut_cubic(uint8_t *lut, uint8_t bit_depth, double shadow)
{
    /* Cubic equation of the form f(x) = Ax^3 + Bx^2 + Cx
     * C is the shadow parameter
     *
     * The roots of the derivative of the cubic must be before 0 and after 1.
     * For a filmic type desaturation and compression of highlights, we choose
     * to place the higher root at 1.01. For f(1) to equal 1, A + B + C = 1.
     * We now have two constraints, we can solve for A and B.
     *
     * We can derive the following equations to calculate suitable A and B values
     * given a particular C in range (0, 3):
     *
     * A = (C - 1.98) / 1.02
     * B = 1 - A - C
     */
    double G = 8.0 / bit_depth;
    double i_scale = 1.0 / (1 << bit_depth);

    assert(bit_depth >= 8);
    assert(bit_depth <= 16);

    // constrain to numerically valid values to ensure 0 < f(x) < 1 in x range (0, 1)
    if (shadow > 2.999) shadow = 2.999;
    if (shadow < 0.001) shadow = 0.001;

    double A = (shadow - 1.98) / 1.02;
    double B = 1 - A - shadow;

    for (int i = 0; i < 1 << bit_depth; i++) {
        double x = i * i_scale;
        double y = A*x*x*x + B*x*x + shadow*x;
        lut[i] = 255.999 * pow(y, G) * black_crush(y);
    }
}

// apply a filmic base curve before gamma encoding
// shadow is the slope at the black end
// gamma controls how midtones are boosted and highlights compressed
// suggested coefficients: gamma=0.3, shadow=0.8
void gamma_gen_lut_filmic(uint8_t *lut, uint8_t bit_depth,
        double gamma, double shadow)
{
    double G = 8.0 / bit_depth;
    double i_scale = 1.0 / (1 << bit_depth);

    /* base curve function: y = a*f(x) + shadow*g(x)
     * y must be in [0, 1] for x in [0, 1]
     *
     * f(x) = x^(gamma/x)
     *
     * let k = 0.05
     * a = 1 + shadow/ln(k)
     * a is defined to ensure that y=1 when x=1
     *
     * g(x) = b * (1 - (k^x))
     * b = -1 / ((1 - k) * ln(k))
     * b is defined to ensure d/dx g(x) = 1 when x=0
     * with this b definition, g(1) = -1/ln(k)
     * g(x) goes from 0 to -1/ln(k), as x goes from 0 to 1
     * slope of g(x) at x=0 is 1
     */
    const double k = 0.05;
    double ln_k = log(k);

    // bound shadow so that shadow * g(1) < 1
    if (shadow >= -ln_k) shadow = ln_k * -0.999;

    double a = 1 + shadow/ln_k;
    double b = -1 / ((1-k) * ln_k);
    double b_shadow = b * shadow;

    assert(bit_depth >= 8);
    assert(bit_depth <= 16);
    assert(gamma > 0);
    assert(shadow >= 0);

    for (int i = 0; i < 1 << bit_depth; i++) {
        double x = i * i_scale;
        double y = a*pow(x, gamma/x) + b_shadow*(1 - pow(k, x));
        lut[i] = 255.999 * pow(y, G) * black_crush(y);
    }
}

// assumes length of lut >= highest value in img_in
void gamma_encode(const uint16_t *img_in, uint8_t *img_out, uint16_t width, uint16_t height,
        const uint8_t *lut)
{
    for (uint32_t i = 0; i < width * height * 3; i++)
        img_out[i] = lut[img_in[i]];
}
