#include "colour_xfrm.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

void print_mat(const ColourMatrix *cmat)
{
    for (int i = 0; i < 9; i += 3)
        printf("%8.5f %8.5f %8.5f\n", cmat->m[i], cmat->m[i+1], cmat->m[i+2]);
}

void cmat_d2f(const ColourMatrix *cmat, ColourMatrix_f *cmat_f)
{
    for (int i = 0; i < 9; i++)
        cmat_f->m[i] = cmat->m[i];
}

/* generate a colour correction matrix
 *
 * exposure:change in stops
 * red:     ratio to multiply red channel by
 * blue:    ratio to multiply blue channel by
 * hue:     hue adjustment in radians
 * sat:     saturation is multiplied by this value
 *          1.0 means no change to saturation
 */
void colour_matrix(ColourMatrix *cmat, double exposure, double red, double blue,
        double hue, double sat)
{
    ColourMatrix wb_mat, sat_mat, hue_mat, work_mat;

    double exp_factor = pow(2, exposure);

    // white balance and exposure
    memset(&wb_mat, 0, sizeof(ColourMatrix));
    wb_mat.m[0] = red * exp_factor;
    wb_mat.m[4] = exp_factor;
    wb_mat.m[8] = blue * exp_factor;

    // saturation
    const double desat = 1 - sat;
    sat_mat.m[0] = 1;
    sat_mat.m[1] = desat;
    sat_mat.m[2] = desat;
    sat_mat.m[3] = desat;
    sat_mat.m[4] = 1;
    sat_mat.m[5] = desat;
    sat_mat.m[6] = desat;
    sat_mat.m[7] = desat;
    sat_mat.m[8] = 1;

    // hue shift is rotating colour vector around 255,255,255 axis in RGB space
    // https://stackoverflow.com/questions/8507885/shift-hue-of-an-rgb-color
    const double cosA = cos(hue);
    const double p = (1.0 - cosA) / 3.0;
    const double q = sqrt(1.0 / 3) * sin(hue);
    const double a = cosA + p;
    const double b = p - q;
    const double c = p + q;
    hue_mat.m[0] = a;
    hue_mat.m[1] = b;
    hue_mat.m[2] = c;
    hue_mat.m[3] = c;
    hue_mat.m[4] = a;
    hue_mat.m[5] = b;
    hue_mat.m[6] = b;
    hue_mat.m[7] = c;
    hue_mat.m[8] = a;

    // first adjust white balance in camera RGB space, then rotate hue, then saturation
    colour_matmult33(&work_mat, &sat_mat, &hue_mat);
    colour_matmult33(cmat, &work_mat, &wb_mat);
}

/* generate a colour correction matrix
 *
 * exposure:change in stops
 * temp_K:  illuminant temperature in Kelvin (convert to D65)
 * tint:    positive boosts red/blue, negative boosts green
 * hue:     hue adjustment in radians
 * sat:     saturation is multiplied by this value
 *          1.0 means no change to saturation
 */
void colour_matrix2(ColourMatrix *cmat, double exposure, double temp_K, double tint,
        double hue, double sat)
{
    // While resulting red and blue ratios may be less than 1, they are already heavily
    // boosted by camera to sRGB matrix, so generally not saturating channels won't be an issue.
    double R, B;
    colour_temp_to_rb_ratio(temp_K, &R, &B);
    double tint_ratio = pow(2, tint);
    R *= tint_ratio;
    B *= tint_ratio;

    colour_matrix(cmat, exposure, R, B, hue, sat);
}

// convert 16-bit integer to floating point image
void colour_i2f(const uint16_t *img_in, float *img_out, uint16_t width, uint16_t height)
{
    for (uint32_t i = 0; i < width * height * 3; i++)
        img_out[i] = img_in[i];
}

// convert floating point image to 16-bit integer
// if bound != 0, also ensure 0 <= pixel_value <= bound
void colour_f2i(const float *img_in, uint16_t *img_out, uint16_t width, uint16_t height, uint16_t bound)
{
    if (bound == 0) {
        for (uint32_t i = 0; i < width * height * 3; i++)
            img_out[i] = img_in[i];
    } else {
        for (uint32_t i = 0; i < width * height * 3; i++) {
            float v = img_in[i];
            if (v < 0)
                img_out[i] = 0;
            else if (v > bound)
                img_out[i] = bound;
            else
                img_out[i] = v;
        }
    }
}

// apply a colour matrix transformation to every pixel in the image
void colour_xfrm(const float *img_in, float *img_out, uint16_t width, uint16_t height,
        const ColourMatrix_f *cmat)
{
    const ColourPixel *imgp_in = (const ColourPixel *)img_in;
    ColourPixel *imgp_out = (ColourPixel *)img_out;

    for (uint32_t i = 0; i < width * height; i++)
        pixel_xfrm(imgp_in + i, imgp_out + i, cmat);
}

// C = A * B
void colour_matmult33(ColourMatrix *C, const ColourMatrix *A, const ColourMatrix *B)
{
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            C->m[row*3 + col] = A->m[row*3] * B->m[col] +
                                A->m[row*3 + 1] * B->m[col + 3] +
                                A->m[row*3 + 2] * B->m[col + 6];
        }
    }
}

void colour_matinv33(ColourMatrix *inv, const ColourMatrix *mat)
{
    double det = mat->m[0] * (mat->m[4] * mat->m[8] - mat->m[7] * mat->m[5]) -
                 mat->m[1] * (mat->m[3] * mat->m[8] - mat->m[5] * mat->m[6]) +
                 mat->m[2] * (mat->m[3] * mat->m[7] - mat->m[4] * mat->m[6]);

    double invdet = 1 / det;

    inv->m[0] = (mat->m[4] * mat->m[8] - mat->m[7] * mat->m[5]) * invdet;
    inv->m[1] = (mat->m[2] * mat->m[7] - mat->m[1] * mat->m[8]) * invdet;
    inv->m[2] = (mat->m[1] * mat->m[5] - mat->m[2] * mat->m[4]) * invdet;
    inv->m[3] = (mat->m[5] * mat->m[6] - mat->m[3] * mat->m[8]) * invdet;
    inv->m[4] = (mat->m[0] * mat->m[8] - mat->m[2] * mat->m[6]) * invdet;
    inv->m[5] = (mat->m[3] * mat->m[2] - mat->m[0] * mat->m[5]) * invdet;
    inv->m[6] = (mat->m[3] * mat->m[7] - mat->m[6] * mat->m[4]) * invdet;
    inv->m[7] = (mat->m[6] * mat->m[1] - mat->m[0] * mat->m[7]) * invdet;
    inv->m[8] = (mat->m[0] * mat->m[4] - mat->m[3] * mat->m[1]) * invdet;
}

// Daylight illuminant temperature to CIE 1931 xy chromaticity
void colour_temp_to_xy(double temp_K, double *x, double *y)
{
    // Officially for 4000K to 25000K, but I stretch it a bit
    // Not super accurate below 3000K but close enough down to 2500K
    if (temp_K < 2500) temp_K = 2500;
    if (temp_K > 50000) temp_K = 50000;

    // https://en.wikipedia.org/wiki/Standard_illuminant#Illuminant_series_D
    double a, b, c, d;
    if (temp_K <= 7000) {
        a = 0.244063;
        b = 99.11;
        c = 2.9678e6;
        d = -4.6070e9;
    } else {
        a = 0.237040;
        b = 247.48;
        c = 1.9018e6;
        d = -2.0064e9;
    }

    *x = a + b/temp_K + c/(temp_K * temp_K) + d/(temp_K*temp_K*temp_K);
    *y = -3*(*x)*(*x) + 2.870*(*x) - 0.275;
}

// Outputs ratios to multiply sRGB red and blue channels by to correct from specified
// illuminant (x,y) to native sRGB D65
void colour_illum_xy_to_rb_ratio(double x, double y, double *ratio_R, double *ratio_B)
{
    // Y = 1.0, convert xyY to XYZ
    double X = x/y;
    double Z = (1.0 - x - y) / y;

    // now convert XYZ to sRGB
    double R = 3.2406*X - 1.5372 - 0.4986*Z;
    double G = -0.9689*X + 1.8758 + 0.0415*Z;
    double B = 0.0557*X - 0.2040 + 1.0570*Z;

    *ratio_R = G/R;
    *ratio_B = G/B;
}

// Outputs ratios to multiply sRGB red and blue channels by to correct from specified
// correlated colour temperature (in Kelvin) to native sRGB D65
void colour_temp_to_rb_ratio(double temp_K, double *ratio_R, double *ratio_B)
{
    double x, y;
    colour_temp_to_xy(temp_K, &x, &y);
    colour_illum_xy_to_rb_ratio(x, y, ratio_R, ratio_B);
}
