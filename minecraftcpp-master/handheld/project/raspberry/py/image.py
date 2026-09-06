import Image
import apiutil

def hexToRgb(hex):
    return (hex >> 16) & 255, (hex >> 8) & 255, hex & 255

cgaColors = map(hexToRgb, (
    0x000000,
    0x0000A8,
    0x00A800,
    0x00A8A8,
    0xFF0000,
    0xA800A8,
    0xA85400,
    0xA8A8A8,
    0x545454,
    0x5454FE,
    0x54FE54,
    0x54FEFE,
    0xFE5454,
    0xFE54FE,
    0xFEFE54,
    0xFEFEFE))

cgaPalette = apiutil.flatten(cgaColors)

def _padPalette(values, count):
    v = tuple(values)
    return v + (0, 0, 0) * (count - len(v))

def imageToCga(im):
    p = Image.new("P", (1,1))
    p.putpalette(_padPalette(cgaPalette, 256))
    return im.convert("RGB").quantize(palette=p, kmeans=255)

def imageToWool(im, (w, h)):
    cga = imageToCga(im.resize((w, h)))

