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
// suggested coefficient: shadow=0.5, gamma=0.05
void gamma_gen_lut_cubic(uint8_t *lut, uint8_t bit_depth, double gamma, double shadow)
{
    /* Cubic equation of the form:
     *  f(x) = Ax^3 + Bx^2 + Cx + D
     * Its derivative is:
     *  f'(x) = 3Ax^2 + 2Bx + C
     *
     * Our requirements are:
     *  f(0) = 0
     *  f(1) = 1
     *  f'(0) = shadow
     *  f'(1) = gamma
     *
     * Substituting in our requirements into the equations, we get:
     *  A = gamma + shadow - 2
     *  B = 3 - gamma - 2*shadow
     *  C = shadow
     *  D = 0
     *
     * To ensure f(x) is in [0, 1] for x in [0,1]:
     *  0 <= shadow <= 3
     *  0 <= gamma <= [an upper bound]
     *
     * While gamma > 1 is mathematically acceptable in some scnearios, it doesn't
     * make much sense photographically, so we will bound gamma <= 1;
     */
    double G = 8.0 / bit_depth;
    double i_scale = 1.0 / (1 << bit_depth);

    assert(bit_depth >= 8);
    assert(bit_depth <= 16);

    // numerical constraints
    if (shadow < 0) shadow = 0;
    if (shadow > 3) shadow = 3;
    if (gamma < 0) gamma = 0;
    if (gamma > 1) gamma = 1;

    double A = gamma + shadow - 2;
    double B = 3 - gamma - 2*shadow;

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

    // shadow must be positive
    if (shadow < 0) shadow = 0;

    // bound gamma to be vaguely reasonable
    if (gamma < 0.001) gamma = 0.001;
    if (gamma > 5) gamma = 5;

    double a = 1 + shadow/ln_k;
    double b = -1 / ((1-k) * ln_k);
    double b_shadow = b * shadow;

    assert(bit_depth >= 8);
    assert(bit_depth <= 16);

    for (int i = 0; i < 1 << bit_depth; i++) {
        double x = i * i_scale;
        double y = a*pow(x, gamma/x) + b_shadow*(1 - pow(k, x));
        lut[i] = 255.999 * pow(y, G) * black_crush(y);
    }
}

// numerically estimate k for (1 - k^x)/(1 - k) to get desired shadow slope at x=0+
static double hdr_shadow_k(double shadow)
{
    /* shadow = ln(1/k) / (1 - k)
     * k = W(-s * e^-s) / s where W is Lambert W function
     * We can't easily calculate this exactly, so numerical approach is needed
     */

    // likewise, base curve becomes unreasonable below shadow=0.001 (blacks crushed)
    if (shadow < 0.001) shadow = 0.001;

    // division by zero at shadow=1 (since k=1), so avoid it
    if (shadow > 0.999 && shadow < 1.001) shadow = 0.999;

    /* define a new variable y to make solving easier
     * let k = e^-y
     * shadow = y / (1 - e^-y)
     *
     * For shadow > 1, an excellent approximation is:
     * y = shadow - 0.42^(shadow - 1)
     *
     * For shadow < 1, a good approximation is:
     * y = 1.95 * ln(shadow) * shadow^0.1
     */

    double y;
    if (shadow > 1)
        y = shadow - pow(0.42, shadow - 1.0);
    else
        y = 1.95 * log(shadow) * pow(shadow, 0.1);

    return exp(-y);
}

// apply the base curve x^gamma * (1 - k^x)/(1 - k) before gamma encoding
// shadow slope is approximately 0.03^gamma * ln(1/k) / (1 - k)
// allows extreme shadow boosting while preserving highlights
// suggested values: gamma=0.1, shadow=[2 to 64] depending on dynamic range
void gamma_gen_lut_hdr(uint8_t *lut, uint8_t bit_depth, double gamma, double shadow)
{
    // bound gamma for reasonableness
    if (gamma < 0.001) gamma = 0.001;
    else if (gamma > 0.99) gamma = 0.99;

    double k = hdr_shadow_k(shadow / pow(0.03, gamma));
    double G = 1.0 / (1.0 - k);
    double i_scale = 1.0 / (1 << bit_depth);

    for (int i = 0; i < 1 << bit_depth; i++) {
        double x = i * i_scale;
        double y = G * (1.0 - pow(k, x)) * pow(x, gamma);
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
