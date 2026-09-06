def identifier():
    return "[a-zA-Z_][0-9a-zA-Z_]*"

def wholeWord(x):
    return "\\b%s\\b"%x

def group(x, name=None):
    if name:
        return "(?P<%s>%s)"%(name, x)
    else:
        return "(%s)"%x