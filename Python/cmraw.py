import struct
import numpy
import cv2
from sys import argv

def unpack12_16(packed):
    arr = numpy.zeros(len(packed)//3 * 2, dtype=numpy.uint16)
    for i in range(len(packed) // 3):
        arr[i*2] = packed[i*3] + ((packed[i*3 + 1] & 0x0F) << 8)
        arr[i*2 + 1] = (packed[i*3 + 1] >> 4) + (packed[i*3 + 2] << 4)
    return arr

def cmraw_load(fname):
    with open(fname, 'rb') as f:
        data = f.read()
    hdr = data[:164]
    if hdr[:4] != b'CMVi':
        raise ValueError("Invalid header magic")
    if hdr[4] != 9:
        raise ValueError("Unsupported pixel format")
    width, height = struct.unpack("<HH", hdr[8:12])
    bayer = unpack12_16(data[164:])
    img12 = cv2.cvtColor(bayer.reshape((height, width)), cv2.COLOR_BayerBG2RGB)
    return img12 / 4095.0

if __name__ == "__main__":
    img = cmraw_load(argv[1])
    cv2.imshow(argv[1], img)
    cv2.waitKey(0)
    cv2.destroyAllWindows()
