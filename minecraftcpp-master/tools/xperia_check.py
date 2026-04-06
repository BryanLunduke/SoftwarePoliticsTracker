# tools for making sure nothing is wrongly set

from xml.dom.minidom import parse, parseString

AName = "android:name"

def hasPermissions(Perms):
    def _hasPermissions(root):
        perms = Perms[:]
        elems = root.getElementsByTagName("uses-permission")
        failed = []
        for e in elems:
            if e.hasAttribute(AName):
                attr = e.getAttribute(AName)
                try:    perms.remove(attr)
                except: failed.append(attr)
        success = len(perms) == 0
        if not success:
            print "Permissions failed:\n\t%s"%",\n\t".join(perms)
        return success
    return _hasPermissions

def testXperiaOptimized(root):
    elem = root.getElementsByTagName("application")[0]
    metas = elem.getElementsByTagName("meta-data")
    for x in metas:
        if x.hasAttribute(AName):
            if x.getAttribute(AName) == "xperiaplayoptimized_content":
                return True
    return False

def testManifestHeader(root):
    package = root.getAttribute("package")
    verCode = root.getAttribute("android:versionCode")
    verName = root.getAttribute("android:versionName")

    print "\n - Package     : %s"%package
    print " - Version Code: %s"%verCode
    print " - Version Name: %s"%verName

    try:
        if root.getAttribute("android:installLocation") != "preferExternal":
            raise Error
    except: print "Warning: android:installLocation is not 'preferExternal"

    return True

def testXperiaOnly(root):
    elem = root.getElementsByTagName("application")[0]
    libs = elem.getElementsByTagName("uses-library")
    for x in libs:
        if x.getAttribute(AName) == "xperiaplaycertified" and \
           x.getAttribute("android:required") == "true":
                return True
    return False

def testPlaystationOnly(root):
    elem = root.getElementsByTagName("application")[0]
    libs = elem.getElementsByTagName("uses-library")
    for x in libs:
        if x.getAttribute(AName) == "playstationcertified" and \
           x.getAttribute("android:required") == "true":
                return True
    return False

def testNameAndIcon(root):
    elem = root.getElementsByTagName("application")[0]
    metas = elem.getElementsByTagName("meta-data")
    hasName = hasIcon = False
    for x in metas:
        if x.hasAttribute(AName):
            if x.getAttribute(AName) == "game_display_name": hasName = True
            if x.getAttribute(AName) == "game_icon": hasIcon = True
    return hasName and hasIcon

def testDebuggable(root):
    elem = root.getElementsByTagName("application")[0]
    if not elem.hasAttribute("android:debuggable"): return True
    return elem.getAttribute("android:debuggable") == "false"

def testMinSdkVersion(Version):
    def _minSdkVersion(root):
        elem = root.getElementsByTagName("uses-sdk")[0]
        if not elem.hasAttribute("android:minSdkVersion"): return False
        return int(elem.getAttribute("android:minSdkVersion")) == Version
    return _minSdkVersion

def testMetaData(root):
    success = True
    success &= testXperiaOptimized(root)
    success &= testNameAndIcon(root)
    return success

def testRealLicenseCheck(root):
    import os
    p, mfn = os.path.split(fn)
    srcfn = os.path.join(p, "src/com/mojang/minecraftpe/MainActivity.java")
    s = file(srcfn, 'r').read()
    x = s.find("new VerizonLicenseThread")
    if x < 0: return False

    b = s.find("(", x)
    e = s.find(")", b)
    if -1 in (b,e): return False
    args = [arg.strip() for arg in s[b+1:e].split(",")]
    if len(args) != 3:
        print "Couldn't parse new VerizonLicenseThread() arguments"
        return False
    return args[2] == "false"

def verify(doc, rules):
    success = True
    root = doc.documentElement
    for rule in rules:
        print ".",
        try:
            ruleResult = rule(root)
        except:
            ruleResult = False
            import traceback
            print traceback.format_exc()
        if not ruleResult:
            print "  Rule %s failed."%rule
            success = False
    return success

if __name__ == "__main__":
    import sys

    generalPerms = hasPermissions([ "android.permission.INTERNET",
                                    "android.permission.WRITE_EXTERNAL_STORAGE"])
    verizonPerms = hasPermissions([ "android.permission.READ_PHONE_STATE",
                                    "android.permission.START_BACKGROUND_SERVICE",
                                    "com.verizon.vcast.apps.VCAST_APPS_LICENSE_SERVICE"])
    marketPerms  = hasPermissions([ "com.android.vending.CHECK_LICENSE"])

    Google = "google"
    Exclusive = "exclusive"
    Verizon = "verizon"
    General = "general"
    allStores = {
        General:[   generalPerms,
                    testXperiaOptimized,
                    testDebuggable,
                    testMinSdkVersion(9),
                    testNameAndIcon,
                    testManifestHeader],
        Google: [   marketPerms],
        Verizon:[   verizonPerms,
                    testRealLicenseCheck],
        Exclusive: [testXperiaOnly,
                    testPlaystationOnly]
    }

    try:    fn = sys.argv[1]
    except: fn = r'C:\dev\subversion\mojang\minecraftcpp\trunk\handheld\project\android\AndroidManifest.xml'

    try:    stores = sys.argv[2].lower().split(",")
    except: stores = allStores.keys()
    if not General in stores: stores.append(General)

    print "Manifest file: %s\n"%fn

    dom = parse(fn)
    root = dom.documentElement
    #print root

    for s in stores:
        if s not in allStores:
            print "Code '%s' not found"%s
            continue
        print "Running through '%s'"%s
        rules = allStores[s]
        if verify(dom, rules):
            print "Success"
        print "\n"
        
    print """
    *   Raise version number (versionName and versionCode)
    *   Sign the application with same key as always
    *   Run zip-align on the signed apk
    """
