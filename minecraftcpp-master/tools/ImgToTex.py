#converts an image to a C array with the given name
#width and height are saved in <name>_w and <name>_h

import sys
import Image
import array

def emitImageTex(imgfn, outfn):
    img = Image.open(imgfn).convert("RGBA")
    pixdata = list(img.getdata())
    data = []
    for pix in pixdata:
        data.extend( map(chr, pix) )

    fp = open(outfn, "wb")
    header = array.array("i", [img.size[0], img.size[1], 0])
    header.tofile(fp)

    arr = array.array("c", data)
    arr.tofile(fp)
    
    fp.close()

if __name__ == "__main__":
    import os

    try:
        fn = sys.argv[1]
        try:
            fnout = sys.argv[2]
        except:
            fnout = os.path.splitext(fn)[0] + ".tex"
    except:
        print("Usage: %s <img-filename> <out-filename>"%sys.argv[0])
        sys.exit(1)

    emitImageTex( fn, fnout )
