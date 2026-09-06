#converts an image to a C array with the given name
#width and height are saved in <name>_w and <name>_h

import sys
import Image

def emitDataArray(varname, arr, perLine=12):
    size = len(arr)
    l = ['unsigned char %s[%d] = {\n'%(varname, size)]
    for (z, ch) in enumerate(arr):
        l.append(("%s,"%hex(ord(ch))))
        #l.append(("%d,"%ord(ch)))
        if (z+1) % perLine == 0: l.append("\n");
    l.append("\n};")
    return "".join(l)

def emitImgDataArray(varname, imgfn, perLine=12):
    img = Image.open(imgfn)
    pixdata = list(img.getdata())
    data = []
    for pix in pixdata:
        data.extend( map(chr, pix) )

    l = ["const int %s_W = %d;\nconst int %s_H = %d;\n"%(varname, img.size[0], varname, img.size[1])]
    l.append( emitDataArray(varname, "".join(data)) )
    return "\n".join(l)

if __name__ == "__main__":
    try:
        fn = sys.argv[1]
        varname = sys.argv[2]
    except:
        print("Usage: %s <img-filename> <out-variable-name>"%sys.argv[0])
        sys.exit(1)

    print emitImgDataArray( varname, fn )
