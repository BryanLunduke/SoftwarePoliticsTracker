import Image

class TileReader:
    def __init__(self, im, size):
        self.im = im
        self.size = size

    def isEmpty(self, x, y):
#        if x == y == 0:
#            print "LOL"
        x *= self.size
        y *= self.size
        for yo in range(self.size):
            for xo in range(self.size):
                try:
                    p = self.im.getpixel((x+xo, y+yo))
                    psum = sum(p)
                    if psum > 0 and p != (255, 0, 255): return False
                except:
                    print self.size, x+xo, y+yo
        return True

    def getPoint(self, x, y):
        return x*self.size, y*self.size

    def getBox(self, x, y):
        x0, y0 = self.getPoint(x, y)
        x1, y1 = self.getPoint(x+1, y+1)
        return x0,y0,x1,y1


def buildImage(listDef, blockImg, iconImg, terrainImg):
    ss = [48, 16, 16]
    ts = [TileReader(blockImg, ss[0]), TileReader(iconImg, ss[1]), TileReader(terrainImg, ss[2])]
    ks = [int( 512 / x ) for x in ss]
    ix = [-1, ks[1]*3*8 -1]

    out = Image.new("RGBA", (512, 512), (0,0,0,0))

    n = 0
    map = {}

    for yy in range(blockImg.size[1] / ss[0]):
        for xx in range(16):#1 + (im.size[0]-1)/48):
            if n >= len(listDef):
                break

            type,b,c,d,e = listDef[n]
            ix[type] += 1
            ii = ix[type]

            x,y = xx, yy
            if type == 1:
                x = e%16
                y = e/16
                if c < 256: type = 2

            sz= ss[type]
            t = ts[type]
            k = ks[type]
            im =t.im

            if not t.isEmpty(x, y):
                box = t.getBox(x, y)
                part = im.crop(box)

                ppt = (sz*(ii%k), sz*(ii/k))
                part.save("t/%s.png"%n)
                out.paste(part, ppt)
            n += 1

    for y in range(out.size[1]):
        for x in range(out.size[0]):
            p = r,g,b,a = out.getpixel((x, y))
            if p == (255,0,255,255):
                out.putpixel((x, y), (0, 0, 0, 0))
    return out

def run(listDef, fn):
    im = Image.open(fn)
    im2 = Image.open("items.png")
    im3 = Image.open("terrain.png")

    buildImage(listDef, im, im2, im3).save("genblocks.png")

"""
if __name__ == "__main__":
    import sys
    try:    fn = sys.argv[1]
    except: fn = "blocks_alpha.png"

#    run(0, fn)
"""