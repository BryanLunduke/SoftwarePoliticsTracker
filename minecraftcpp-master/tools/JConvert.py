import re, rehelper as h, path

SUB = 1
IF_PARAMETER = 11
FUNC_PER_ROW  = 101
FUNC_PER_FILE = 102

class Params:
    def __init__(self, regexprules, *other):
        self.regexps = []
        self.other = other
        
        self.addRegexps(regexprules)

    def addRegexps(self, rules):
        self.regexps.extend(rules)

def GetPackageInfo(x):
    try:
        pkg = re.search("package (.*?);", x, re.M)
        return pkg.span()[0], pkg.groups()[0].split(".")
    except: return -1, []

def ImportToInclude(env, x):
    if not x.startswith("import"):
        return x

    x = x.rstrip().rstrip(";")
    if x.endswith("*"):
        return "/* " + x + " */"

    packages = x[7:].replace(".", "/")
    basepath = "/".join(env['package'])
    relpath = path.Path(basepath).relpathto(packages).replace("\\", "/")

    """
    print packages
    print basepath
    print relpath
    raw_input()
    """

    return '#include "' + relpath + '.h"';

def PackageToGuards(x):
    try:
        b, pkg = GetPackageInfo(x)
        if b == -1: raise Exception("Package not found")
        package = "_".join(pkg)
        x = x[:b] + "//" + x[b:]
        clazz = re.search("(class|interface) (%s)"%h.identifier(), x, re.M).groups()[1]
        full = "%s__%s_H__"%(package.upper(), clazz)
        return "#ifndef %s\n#define %s\n\n%s\n#endif /*%s*/\n"%(full,full,x,full)
    except:
        import traceback
        traceback.print_exc()
        return x

def convert(infilename, outfilename, params):
    ins = outs = file(infilename, 'r').read()
    env = {"package": GetPackageInfo(outs)[1]}

    for ot, of in params.other:
        if ot == FUNC_PER_ROW:
            l = outs = "\n".join([of(env, line) for line in outs.split("\n")])

        if ot == FUNC_PER_FILE: outs = of(outs)

    def _ifParameter(dst):
        def checkIfParam(b, e):
            eaten = []
            for x in outs[:b][::-1]:
                if x == "(": break
                if x in "){}": return False
                eaten.append(x)
            for x in outs[e+1:]:
                if x == ")": break
                if x in "({}": return False
                eaten.append(x)
            return "=" not in eaten
        return lambda m: dst if checkIfParam(*m.span()) else m.group()
    F = {IF_PARAMETER : _ifParameter
    }
    for rexp in params.regexps:
        type, src, dst = rexp[:3]
        try: dstFunc = F.get(rexp[3], None)(dst)
        except: dstFunc = None
        if type == SUB: outs = re.sub(src, dstFunc or dst, outs)

    file(outfilename, 'w').write(outs)

def compileRegexpRules(ruleList): return ruleList

JavaToCppRules = compileRegexpRules([
    (SUB, h.wholeWord("boolean"),   "bool"),
    (SUB, h.wholeWord("double"),    "float"),
    (SUB, h.wholeWord("byte"),      "char"),
    (SUB, h.wholeWord("String"),    "const std::string&", IF_PARAMETER),
    (SUB, h.wholeWord("String"),    "std::string"),
    (SUB, h.wholeWord("null"),      "NULL"),
    (SUB, h.wholeWord("private"),   "/*private*/"),
    (SUB, h.wholeWord("protected"), "/*protected*/"),
    (SUB, h.wholeWord("public"),    "/*public*/"),
    (SUB, h.wholeWord("interface"), "/*interface*/ class"),
    (SUB, h.wholeWord("final"), "const"),
    (SUB, h.wholeWord("new"), "/*new*/"),
    (SUB, h.wholeWord(" implements "),": /*implements-interface*/ public "),
    (SUB, h.wholeWord(" extends "),": public "),
    (SUB, "@Override",  "/*@Override*/"),
    (SUB, "\\bthis[.]", "this->"),
    (SUB, "\\bsuper[.]","super::"),
    (SUB, "\\btag[.]","tag->"),
    (SUB, "\\bentityTag[.]","entityTag->")
])

JavaToCppParams = Params(JavaToCppRules,
                        (FUNC_PER_ROW, ImportToInclude),
                        (FUNC_PER_FILE,PackageToGuards))

if __name__ == "__main__":
    import os, sys
    import glob

    MinecraftRules = compileRegexpRules([
        (SUB, "\\blevel[.]","level->"),
        (SUB, "\\bentity[.]","entity->"),
        (SUB, "\\bplayer[.]","player->"),
        (SUB, h.wholeWord("Mob"), "Mob*", IF_PARAMETER),
        (SUB, h.wholeWord("Player"), "Player*", IF_PARAMETER),
        (SUB, h.wholeWord("Tile"), "Tile*", IF_PARAMETER),
        (SUB, h.wholeWord("Level"), "Level*", IF_PARAMETER),
        (SUB, h.wholeWord("Entity"), "Entity*", IF_PARAMETER),
        (SUB, "\\bTile[.](%s)[.]"%h.identifier(),"Tile::\\1->"),
        (SUB, "\\bItem[.](%s)[.]"%h.identifier(),"Item::\\1->"),
        (SUB, "\\b[^/]?Tile[.]","Tile::"),
        (SUB, "\\b[^/]?Item[.]","Item::"),
        (SUB, "\\b[^/]?Material[.]","Material::"),
        (SUB, "\\b[^/]?Mth[.]","Mth::")
    ])

    # Java to C++
    params = JavaToCppParams
    # add more params here if needed
    params.addRegexps(MinecraftRules)

    fns = glob.glob(sys.argv[1])
    for full in fns:
        fn, ext = os.path.splitext(full)
        outfn = "%s.h"%fn

        if os.path.exists(outfn):
            continue

        convert("%s.java"%fn, outfn, params)
