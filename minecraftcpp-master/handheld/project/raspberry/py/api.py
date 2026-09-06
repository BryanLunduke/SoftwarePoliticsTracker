import socket, select
import apiutil
from vec3 import Vec3
from event import TileEvent

comm = None
def connect(address, port=4711):
    global comm
    comm = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    comm.connect((address, 4711))

connect("localhost")

def drainSocket(s):
    while True:
        read, w, x = select.select([s], [], [], 0.0)
        if len(read) == 0: break
        s.recv(1500)

def generateFunc(name, unpack=None):
    def _inner(*l):
        s = apiutil.call(name,*l)
        drainSocket(comm)
        comm.send(s)
        if not unpack: return

        r = comm.recv(1024)
        if r != "Fail\n":
            return unpack(r)
    return  _inner

def unpackArrayTo(func, f=lambda x:x):
    """Calling func with unpacked array element arguments"""
    def _inner(s): return [func(*map(f, e.split(","))) for e in s.split("|") if e.strip()]
    return _inner

def unpackTo(func, f=lambda x:x):
    """Calling func with unpacked arguments"""
    def _inner(s): return func(*map(f, s.split(",")))
    return _inner

def postMessageLoop():
    n = 0
    while 1:
        postMessage(n)
        n += 1

def Print(s):
    print(s)

def readV3(s):      return unpackTo(Vec3, float)(s)
def readV3ints(s):  return unpackTo(Vec3, lambda x:int(float(x)))(s)

class Holder:
    pass

# Block, world
world = Holder()
world.setBlock          = generateFunc("world.setBlock")
world.getBlock          = generateFunc("world.getBlock", int)
world.setBlocks         = generateFunc("world.setBlocks")
world.getHeight         = generateFunc("world.getHeight", int)
world.getPlayerIds      = generateFunc("world.getPlayerIds", unpackArrayTo(int))
world.setting           = generateFunc("world.setting")

# Checkpoint
world.checkpoint = Holder()
world.checkpoint.save   = generateFunc("world.checkpoint.save")
world.checkpoint.restore= generateFunc("world.checkpoint.restore")
#               = generateFunc("entity.getType

# Player
player = Holder()
player.setTile          = generateFunc("player.setTile")
player.getTile          = generateFunc("player.getTile", readV3ints)
player.setPos           = generateFunc("player.setPos")
player.getPos           = generateFunc("player.getPos", readV3)
player.setting          = generateFunc("player.setting")

# Camera
camera = Holder()
camera.setNormal        = generateFunc("camera.mode.setNormal")
#camera.setThirdPerson   = generateFunc("camera.mode.setThirdPerson")
camera.setFixed         = generateFunc("camera.mode.setFixed")
camera.setFollow        = generateFunc("camera.mode.setFollow")
camera.setPos           = generateFunc("camera.setPos")
#setCameraFollow1 = generateFunc("camera.mode.setFollowWithPosition")
#setCameraFollow2 = generateFunc("camera.mode.setFollowWithRotation")

# Events
events = Holder()
events.block = Holder()
events.block.hits   = generateFunc("events.block.hits", unpackArrayTo(TileEvent.Hit, int))
#pollChat        = generateFunc("events.chat", unpackArrayTo(TileEvent.Hit, int))
# Chat
chat = Holder()
chat.post       = generateFunc("chat.post")

from functools import partial
setAutojump = partial(player.setting, "autojump")