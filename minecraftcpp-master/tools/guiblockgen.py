rl = """1, 0, 273, 0, 81
1, 1, 274, 0, 97
1, 2, 275, 0, 113
1, 3, 359, 0, 93
1, 4, 272, 0, 65
1, 5, 65, 0, 83
1, 6, 50, 0, 80
1, 7, 324, 0, 43
0, 8, 85, 0, 0
0, 9, 107, 0, 0
0, 10, 4, 0, 0
0, 11, 17, 1, 0
0, 12, 17, 2, 0
0, 13, 5, 0, 0
0, 14, 45, 0, 0
0, 15, 3, 0, 0
0, 16, 24, 0, 0
0, 17, 13, 0, 0
0, 18, 1, 0, 0
0, 19, 67, 0, 0
0, 20, 53, 0, 0
0, 21, 108, 0, 0
0, 22, 44, 0, 0
0, 23, 44, 2, 0
0, 24, 44, 4, 0
0, 25, 12, 0, 0
0, 26, 35, 7, 0
0, 27, 35, 6, 0
0, 28, 35, 5, 0
0, 29, 35, 4, 0
0, 30, 35, 3, 0
0, 31, 35, 2, 0
0, 32, 35, 1, 0
0, 33, 35, 15, 0
0, 34, 35, 14, 0
0, 35, 35, 13, 0
0, 36, 35, 12, 0
0, 37, 35, 11, 0
0, 38, 35, 10, 0
0, 39, 35, 9, 0
0, 40, 35, 8, 0
0, 41, 20, 0, 0
0, 42, 18, 0, 0
0, 43, 41, 0, 0
0, 44, 42, 0, 0
0, 45, 57, 0, 0
0, 46, 49, 0, 0
0, 47, 47, 0, 0
0, 48, 58, 0, 0
0, 49, 61, 0, 0
1, 50, 37, 0, 13
1, 51, 38, 0, 12
1, 52, 39, 0, 29
1, 53, 40, 0, 28
1, 54, 81, 0, 70
1, 55, 83, 0, 73
0, 56, 15, 0, 0
0, 57, 14, 0, 0
0, 58, 56, 0, 0
0, 59, 21, 0, 0
1, 60, 266, 0, 39
1, 61, 265, 0, 23
0, 62, 17, 0, 0
1, 63, 263, 1, 7
1, 64, 264, 0, 55
1, 65, 351, 2, 110
1, 66, 337, 0, 57
1, 67, 336, 0, 22
1, 68, 270, 0, 96
1, 69, 280, 0, 53
1, 70, 269, 0, 80
1, 71, 271, 0, 112
1, 72, 257, 0, 98
1, 73, 256, 0, 82
1, 74, 258, 0, 114
1, 75, 278, 0, 99
1, 76, 277, 0, 83
1, 77, 279, 0, 115
1, 78, 285, 0, 100
1, 79, 284, 0, 84
1, 80, 286, 0, 116
1, 81, 268, 0, 64
1, 82, 267, 0, 66
1, 83, 276, 0, 67
1, 84, 283, 0, 68
0, 85, 22, 0, 0
1, 86, 351, 4, 142
1, 87, 102, 0, 49
0, 88, 35, 0, 0
1, 89, 351, 11, 12
1, 90, 339, 0, 58
1, 91, 338, 0, 27
1, 92, 340, 0, 59
0, 93, 80, 0, 0
1, 94, 332, 0, 14
0, 95, 82, 0, 0
0, 96, 44, 3, 0
1, 97, 353, 0, 13
1, 98, 263, 0, 7
1, 99, 281, 0, 71
1, 100, 6, 2, 79
1, 101, 6, 1, 63
1, 102, 6, 0, 15"""

class T:
    def __init__(self):
        self.t = {}
    def set(self, i, id, data):
        if not id in self.t: self.t[id] = []
        self.t[id].append((i,data))
    def _getType(self, id):
        return len(self.t.get(id, []))
    def isAvailable(self, id):
        return 0 != self._getType(id)
    def isUnique(self, id):
        return 1 == self._getType(id)
    def isMultiple(self, id):
        return 2 <= self._getType(id)
    def hasDataFor(self, id, data):
        if not self.isAvailable(id): return False
        return data in self.t.get(id, [(0,0)])
    def getIndexFor(self, id, data):
        for (i, d) in self.t[id]:
            if data == d:
                return i
        return -1

def getCArrayFormat(l):
    #if len(l) < 20:
    return "{" + ", ".join(map(str,l)) + "};"
    s  = "{\n"
    n  = 0
    for x in l:
        if n == 0: s += "    ";
        s += "%3d,"%x
        n += 1
        if n == 10:
            s+= "\n"
            n = 0
    s += "};"
    return s


if __name__ == "__main__":
    def emitDefinition(id, data):
        typeId = "static const signed short"
        return "%s _%s[] = %s\n"%(typeId,id,getCArrayFormat(data))

    ll = [map(int, k.split(", ")) for k in [x.strip() for x in rl.split("\n") if x.strip()]]
    
    import blockplacer
    blockplacer.run(ll, "blocks.png")

    types = {}

    ts = [T(), T()]
    index = [-1, 128-1]
    maxId = 0
    for (type, i, id, data, i2) in ll:
        index[type] += 1
        ts[type].set(index[type], id, data)
        if id > maxId: maxId = id

    NumItems = maxId + 1
    handled = {}
    l = [-1] * NumItems # -1 is HAS_NOT, -2 is RE-QUERY, 0 <= x < 128 is
                        # block index, x >= 128 is icon index
    q = []
    for (type, i, id, data, i2) in ll:
        # type == 0: Rendered Tile. Top left 480 * 432 pixels are allocated for those.
        # type == 1: Icon. Index start on 128
        
        if id >= NumItems: continue

        t = ts[type]
        if id == 38:
            print type, i, id, data, i2,

        if t.isUnique(id):
            l[id] = t.getIndexFor(id, data)
        if t.isMultiple(id) and id not in handled:
            l[id] = -2
            handled[id] = True
            m = []
            for data in range(16): # data
                index = t.getIndexFor(id, data)
                m.append(index)
            q.append( (id, emitDefinition(id, m)))

    import sys
    f = file("def.cpp", 'w')
    sys.stdout = f

    for (id, datalist) in q:
        print datalist
    print emitDefinition("mapper", l)
    #print l[45]