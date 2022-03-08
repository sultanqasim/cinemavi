#include "colour_xfrm.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "cie_xyz.h"

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
 * red:     ratio to multiply red channel by
 * blue:    ratio to multiply blue channel by
 * hue:     hue adjustment in radians
 * sat:     saturation is multiplied by this value
 *          1.0 means no change to saturation
 */
void colour_matrix(ColourMatrix *cmat, double red, double blue, double hue, double sat)
{
    ColourMatrix wb_mat, sat_mat, hue_mat, work_mat;

    // white balance
    memset(&wb_mat, 0, sizeof(ColourMatrix));
    wb_mat.m[0] = red;
    wb_mat.m[4] = 1.0;
    wb_mat.m[8] = blue;

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
    const ColourPixel_f *imgp_in = (const ColourPixel_f *)img_in;
    ColourPixel_f *imgp_out = (ColourPixel_f *)img_out;

    for (uint32_t i = 0; i < width * height; i++)
        pixel_xfrm_f(imgp_in + i, imgp_out + i, cmat);
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
void colour_temp_tint_to_xy(double temp_K, double tint, double *x, double *y)
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

    double x0 = a + b/temp_K + c/(temp_K * temp_K) + d/(temp_K*temp_K*temp_K);
    double y0 = -3*x0*x0 + 2.870*x0 - 0.275;

    /* We want to adjust tint along isotherm, so n needs to be kept constant
     * where n = (x - x_e) / (y - y_e), x_e = 0.3366, y_e = 0.1725
     *
     * Let's define tint as 10x the distance in xy coords along isotherm
     * delta_y = 0.1 * tint / sqrt(1 + n^2)
     * y = y0 + delta_y
     * (x - x_e) / (y0 + delta_y - y_e) = (x0 - x_e) / (y0 - y_e)
     *
     * Solving for x, we get:
     * x = (x0*(y - y_e) - x_e*delta_y) / (y0 - y_e)
     */
    if (tint != 0) {
        double n = (x0 - 0.3366) / (y0 - 0.1725);
        double delta_y = 0.1 * tint / sqrt(1 + n*n);
        *y = y0 + delta_y;
        *x = (x0*(*y - 0.1725) - 0.3366*delta_y) / (y0 - 0.1725);
    } else {
        *x = x0;
        *y = y0;
    }
}

// CIE 1931 xy chromaticity to CCT
void colour_xy_to_temp_tint(double x, double y, double *temp_K, double *tint)
{
    // https://en.wikipedia.org/wiki/Color_temperature#Approximation
    // Hernández-Andrés approximation from xy to temp_K
    double n = (x - 0.3366) / (y - 0.1735);
    *temp_K = -949.86315 + 6253.80338*exp(n/-0.92159) + 28.70599*exp(n/-0.20039) + 0.00004*exp(n/-0.07125);

    // find xy distance from daylight coords at calculated temp
    // tint ix 10x this distance as per our definition
    double day_x, day_y;
    colour_temp_tint_to_xy(*temp_K, 0, &day_x, &day_y);
    double delta_x, delta_y;
    delta_x = x - day_x;
    delta_y = y - day_y;
    *tint = 10.0 * sqrt(delta_x*delta_x + delta_y*delta_y) * delta_y/fabs(delta_y);
}

// Outputs ratios to multiply source red and blue channels by to correct from specified
// illuminant (x,y)
void colour_illum_xy_to_rb_ratio(const ColourMatrix *src_to_xyz,
        double x, double y, double *ratio_R, double *ratio_B)
{
    // Y = 1.0, convert xyY to sRGB
    ColourPixel XYZ = {.p={x/y, 1.0, (1.0 - x - y) / y}};
    ColourPixel RGB;
    ColourMatrix xyz_to_cam;
    colour_matinv33(&xyz_to_cam, src_to_xyz);
    pixel_xfrm(&XYZ, &RGB, &xyz_to_cam);

    *ratio_R = RGB.p[1]/RGB.p[0];
    *ratio_B = RGB.p[1]/RGB.p[2];
}

// inverse of colour_illum_xy_to_rb_ratio
void colour_rb_ratio_to_illum_xy(const ColourMatrix *src_to_xyz,
        double ratio_R, double ratio_B, double *x, double *y)
{
    ColourPixel RGB = {.p={1/ratio_R, 1, 1/ratio_B}};
    ColourPixel XYZ;
    pixel_xfrm(&RGB, &XYZ, src_to_xyz);

    *x = XYZ.p[0] / (XYZ.p[0] + XYZ.p[1] + XYZ.p[2]);
    *y = XYZ.p[1] / (XYZ.p[0] + XYZ.p[1] + XYZ.p[2]);
}

// Outputs ratios to multiply source red and blue channels by to correct from specified
// correlated colour temperature (in Kelvin)
void colour_temp_tint_to_rb_ratio(const ColourMatrix *src_to_xyz,
        double temp_K, double tint, double *ratio_R, double *ratio_B)
{
    double x, y;
    colour_temp_tint_to_xy(temp_K, tint, &x, &y);
    colour_illum_xy_to_rb_ratio(src_to_xyz, x, y, ratio_R, ratio_B);
}

// inverse of colour_temp_tint_to_rb_ratio
void colour_rb_ratio_to_temp_tint(const ColourMatrix *src_to_xyz,
        double ratio_R, double ratio_B, double *temp_K, double *tint)
{
    double x, y;
    colour_rb_ratio_to_illum_xy(src_to_xyz, ratio_R, ratio_B, &x, &y);
    colour_xy_to_temp_tint(x, y, temp_K, tint);
}

// determine camera space values to hit (1.0, 1.0, 1.0) in target space
void colour_white_in_cam(const ColourMatrix *target_to_cam, ColourPixel *cam_white)
{
    ColourPixel full_white = {.p={1.0, 1.0, 1.0}};
    pixel_xfrm(&full_white, cam_white, target_to_cam);
}

// scale matrix (in place) to ensure white in target space is achievable in camera space
// exposure is a boost or reduction in stops, 0 is no change
void colour_matrix_white_scale(ColourMatrix *cam_to_target, double exposure)
{
    double exp_factor = pow(2, exposure);
    ColourMatrix target_to_cam;
    ColourPixel cam_white;
    colour_matinv33(&target_to_cam, cam_to_target);
    colour_white_in_cam(&target_to_cam, &cam_white);

    double max_chan = -1E6;
    for (int i = 0; i < 3; i++)
        if (cam_white.p[i] > max_chan)
            max_chan = cam_white.p[i];

    exp_factor *= max_chan;

    for (int i = 0; i < 9; i++)
        cam_to_target->m[i] *= exp_factor;
}

// pre-clip integer camera RGB values (in place) to ensure transformed clipped whites stay white
void colour_pre_clip(uint16_t *img, uint16_t width, uint16_t height, uint16_t max_val,
        const ColourMatrix *cam_to_target)
{
    ColourMatrix target_to_cam;
    ColourPixel cam_white;
    colour_matinv33(&target_to_cam, cam_to_target);
    colour_white_in_cam(&target_to_cam, &cam_white);

    double exp_factor = 0;
    for (int i = 0; i < 3; i++)
        if (cam_white.p[i] > exp_factor)
            exp_factor = cam_white.p[i];

    exp_factor = 1.0 / exp_factor;
    for (int i = 0; i < 3; i++)
        cam_white.p[i] *= exp_factor;

    uint16_t max_R = cam_white.p[0] < 1.0 ? max_val * cam_white.p[0] : max_val;
    uint16_t max_G = cam_white.p[1] < 1.0 ? max_val * cam_white.p[1] : max_val;
    uint16_t max_B = cam_white.p[2] < 1.0 ? max_val * cam_white.p[2] : max_val;

    for (uint32_t i = 0; i < width * height * 3; i += 3) {
        img[i] = img[i] > max_R ? max_R : img[i];
        img[i+1] = img[i+1] > max_G ? max_G : img[i+1];
        img[i+2] = img[i+2] > max_B ? max_B : img[i+2];
    }
}
