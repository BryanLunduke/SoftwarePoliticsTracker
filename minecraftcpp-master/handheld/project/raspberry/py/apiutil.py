import collections

def flatten(l):
    for e in l:
        if isinstance(e, collections.Iterable) and not isinstance(e, basestring):
            for ee in flatten(e): yield ee
        else: yield e

def toParameters(l):
    return ",".join(map(str, flatten(l)))

def call(name, *l):
    return "%s(%s)\n"%(name, toParameters(l))
