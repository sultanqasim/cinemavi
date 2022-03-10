from cmraw import cmraw_load
from colour_checker_detection import detect_colour_checkers_segmentation
from sys import argv
import colour
import numpy
import matplotlib.pyplot as plt

# Take a picture of a 24 swatch X-Rite/Calibrite ColorChecker Classic in D65 light
# Alternatively photograph colourchecker.png on an accurate display with D65 white point
# OLED displays recommended for deeper blacks and better linearity
# Make sure things like TrueTone, Night Shfit, and f.lux are disabled if doing this
# Also make sure the display is clean (free from smudges) for best accuracy
D65 = colour.CCS_ILLUMINANTS['CIE 1931 2 Degree Standard Observer']['D65']
ref_colour_checker = colour.CCS_COLOURCHECKERS['ColorChecker 2005']

ref_swatches = colour.XYZ_to_RGB(
        colour.xyY_to_XYZ(list(ref_colour_checker.data.values())),
        ref_colour_checker.illuminant, D65,
        colour.RGB_COLOURSPACES['sRGB'].matrix_XYZ_to_RGB)

img = cmraw_load(argv[1])
swatches, = detect_colour_checkers_segmentation(img)

# Swatch 18 is white, swatch 23 is dark grey
# Let's plot the linearity
green_x = swatches[18:24, 1]
green_y = ref_swatches[18:24, 1]
green_y *= green_x[0] / green_y[0]
lin = numpy.linspace(0.0, 1.0)
plt.plot(green_x, green_y, "o")
plt.plot(lin, lin)

corr_mat = colour.characterisation.matrix_colour_correction_Cheung2004(
        swatches, ref_swatches)

print("Camera to linear sRGB matrix:")
print(corr_mat)

def colour_xfrm(image, xfrm_mat):
    ishape = image.shape
    image_flat = image.reshape((-1, 3))
    return xfrm_mat.dot(image_flat.transpose()).transpose().reshape(ishape)

# Convert to linear sRGB colour space, then sRGB gamma encode
img_srgb = colour.cctf_encoding(colour_xfrm(img, corr_mat))
colour.plotting.plot_image(img_srgb)
