#include <math.h>
#include <assert.h>
#include "gamma.h"

// x is a luminance value between 0 and 1
static inline uint8_t gamma_encode_srgb(double x)
{
    double encoded;
    if (x <= 0.0031308)
        encoded = 12.92 * x;
    else
        encoded = 1.055 * pow(x, 1/2.4) - 0.055;
    return encoded * 255.1;
}

// generate lut for gamma encoding a linear space image
// bit depth should be between 8 and 16
// length of lut should be 1 << bit_depth
void gamma_gen_lut(uint8_t *lut, uint8_t bit_depth)
{
    double i_scale = 1.0 / ((1 << bit_depth) - 1);

    assert(bit_depth <= 16);

    for (uint16_t i = 0; i < 1 << bit_depth; i++) {
        double x = i * i_scale;
        lut[i] = gamma_encode_srgb(x);
    }
}

// apply cubic base curve before gamma encoding
// suggested coefficients: shadow=0.3, black=0.2
void gamma_gen_lut_cubic(uint8_t *lut, uint8_t bit_depth, double gamma, double black)
{
    /* Cubic equation of the form:
     *  f(x) = Ax^3 + Bx^2 + Cx + D
     * Its derivative is:
     *  f'(x) = 3Ax^2 + 2Bx + C
     *
     * Our requirements are:
     *  f(0) = 0
     *  f(1) = 1
     *  f'(0) = black
     *  f'(1) = gamma
     *
     * Substituting in our requirements into the equations, we get:
     *  A = gamma + black - 2
     *  B = 3 - gamma - 2*black
     *  C = black
     *  D = 0
     *
     * To ensure f(x) is in [0, 1] for x in [0,1]:
     *  0 <= black <= 3
     *  0 <= gamma <= [an upper bound]
     *
     * While gamma > 1 is mathematically acceptable in some scnearios, it doesn't
     * make much sense photographically, so we will bound gamma <= 1;
     */
    double i_scale = 1.0 / ((1 << bit_depth) - 1);

    assert(bit_depth <= 16);

    // numerical constraints
    if (black < 0) black = 0;
    if (black > 3) black = 3;
    if (gamma < 0) gamma = 0;
    if (gamma > 1) gamma = 1;

    double A = gamma + black - 2;
    double B = 3 - gamma - 2*black;

    for (uint16_t i = 0; i < 1 << bit_depth; i++) {
        double x = i * i_scale;
        double y = A*x*x*x + B*x*x + black*x;
        lut[i] = gamma_encode_srgb(y);
    }
}

// apply a filmic base curve before gamma encoding
// gamma controls how midtones are boosted and highlights compressed
// black is the slope at the black end
// suggested coefficients: gamma=0.3, black=0.8
void gamma_gen_lut_filmic(uint8_t *lut, uint8_t bit_depth, double gamma, double black)
{
    double i_scale = 1.0 / ((1 << bit_depth) - 1);

    /* base curve function: y = a*f(x) + black*g(x)
     * y must be in [0, 1] for x in [0, 1]
     *
     * f(x) = x^(gamma/x)
     *
     * let k = 0.05
     * a = 1 + black/ln(k)
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

    // bound black so that black * g(1) < 1
    if (black >= -ln_k) black = ln_k * -0.999;

    // black must be positive
    if (black < 0) black = 0;

    // bound gamma to be vaguely reasonable
    if (gamma < 0.001) gamma = 0.001;
    if (gamma > 5) gamma = 5;

    double a = 1 + black/ln_k;
    double b = -1 / ((1-k) * ln_k);
    double b_black = b * black;

    assert(bit_depth <= 16);

    for (uint16_t i = 0; i < 1 << bit_depth; i++) {
        double x = i * i_scale;
        double y = a*pow(x, gamma/x) + b_black*(1 - pow(k, x));
        lut[i] = gamma_encode_srgb(y);
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
    double i_scale = 1.0 / ((1 << bit_depth) - 1);

    for (uint16_t i = 0; i < 1 << bit_depth; i++) {
        double x = i * i_scale;
        double y = G * (1.0 - pow(k, x)) * pow(x, gamma);
        lut[i] = gamma_encode_srgb(y);
    }
}

// compressed cubic curve with Reinhard tail for shadow boosting while preserving highlights
// gamma is slope at white end
// shadow is how much cubic is compressed to boost shadows
// black is slope at black end before compression of cubic
//
// suggested coefficients:
// gamma = 0.2 for shadow <= 7.5
// gamma = 1.5/shadow for 7.5 < shadow < 37.5
// gamma = 0.04 for shadow >= 37.5
// shadow = 1 to 64
// black = 0.3 for every setting
void gamma_gen_lut_hdr_cubic(uint8_t *lut, uint8_t bit_depth, double gamma, double shadow,
        double black)
{
    /* Let's call our HDR curve h(x)
     *
     * Both the cubic and Reinhard portions meet at x = 1/shadow
     * Cubic portion:
     *  h(x) = Ax^3 + Bx^2 + Cx for x in [0, 1/shadow]
     * Reinhard portion:
     *  h(x) = (1 + r)x / (x + r) for x in [1/shadow, 1]
     *
     * Deriviative of Reninhard is:
     *  R'(x) = r(r + 1) / (r + x)^2
     * Solving for r when R'(1) = gamma:
     *  r = gamma / (1 - gamma)
     *
     * Let Q = h(1/shadow) = (1 + r) * (1/shadow) / (1 + 1/shadow)
     *
     * Let k = black, g = R'(1/shadow) / (Q * shadow)
     * We construct a cubic f(x) from (0,0) to (1,1) using the math in gamma_gen_lut_cubic:
     *  f(x) = A'x^3 + B'x^2 + C'x
     *  A' = g + k - 2
     *  B' = 3 - g - 2*k
     *  C' = k
     *
     * We now compress the cubic f(x) to only occupy the shadow region:
     *  A = A' * Q * shadow^3
     *  B = B' * Q * shadow^2
     *  C = C' * Q * shadow
     */

    assert(bit_depth <= 16);

    double i_scale = 1.0 / ((1 << bit_depth) - 1);

    // numerical constraints
    if (gamma < 0.001) gamma = 0.001;
    if (gamma > 0.999) gamma = 0.999;
    if (shadow < 1) shadow = 1;
    if (black < 0) black = 0;
    if (black > 3) black = 3;

    double inv_shadow = 1.0 / shadow;
    double r = gamma / (1 - gamma);
    double r_1 = r + 1;
    double Q = r_1 * inv_shadow / (r + inv_shadow);
    double Rprime_IS = r * r_1 / ((r+inv_shadow) * (r+inv_shadow));
    double g = Rprime_IS / (shadow * Q);
    double A = Q * shadow*shadow*shadow * (g + black - 2);
    double B = Q * shadow*shadow * (3 - g - 2*black);
    double C = Q * shadow * black;

    for (uint16_t i = 0; i < 1 << bit_depth; i++) {
        double x = i * i_scale;
        double y;
        if (x < inv_shadow)
            y = A*x*x*x + B*x*x + C*x;
        else
            y = r_1 * x / (x + r);
        lut[i] = gamma_encode_srgb(y);
    }
}

// assumes length of lut >= highest value in img_in
void gamma_encode(const uint16_t *img_in, uint8_t *img_out, uint16_t width, uint16_t height,
        const uint8_t *lut)
{
    for (uint32_t i = 0; i < width * height * 3; i++)
        img_out[i] = lut[img_in[i]];
}
