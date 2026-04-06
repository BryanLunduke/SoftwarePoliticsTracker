import time, random
import api, kbhelp
from vec3 import Vec3

class BlockType:
    AIR         = 0
    ROCK        = 1
    DIRT        = 3
    SANDSTONE   = 24
    CLOTH       = 35
    CACTUS      = 81

class ClothColor:
    WHITE       = 0
    RED         = 14

    @staticmethod
    def dark(x):   return x + 8 if x < 8 else x
    def bright(x): return x - 8 if x >= 8 else x

class Level:
    Wall = BlockType.DIRT
    Food = BlockType.SANDSTONE

    def __init__(self, w, h, pos=Vec3(0,0,0)):
        self.w = w
        self.h = h
        self.pos = pos
        self.m = [0] * (w * h)

    def setup(self, numApples):
        size = Vec3(self.w-1, 0, self.h-1)
        margin = Vec3(1, 0, 1)
        api.world.setBlocks(self.pos - margin + Vec3.down(3), self.pos + size + margin, BlockType.ROCK)
        api.world.setBlocks(self.pos, self.pos + size + Vec3.up(8), BlockType.AIR)

        for i in random.sample(range(self.w * self.h), numApples):
            xx = i % self.w
            yy = i / self.w
            self.m[i] = Level.Food
            api.world.setBlock(xx, self.pos.y, yy, Level.Food)

    def getTile(self, x, y):
        return self.m[y * self.w + x] if self.inRange(x, y) else None

    def clearTile(self, x, y):
        if self.inRange(x, y): self.m[y * self.w + x] = 0

    def isFood(self, x, y):
        return self.inRange(x, y) and Level.Food == self.getTile(x, y)

    def isWall(self, x, y):
        return (not self.inRange(x, y)) or Level.Wall == self.getTile(x, y)

    def inRange(self, x, y):
        return x >= 0 and y >= 0 and x < self.w and y < self.h
        
    def countApples(self):
        return self.m.count(Level.Food)

class Snake:
    def __init__(self, pos=Vec3(0,0,0), length=5):
        self.dir = Vec3(1, 0, 0)
        self.seg = [pos]
        self.dead = False
        self.growTicks = 0

        for i in range(length-1):
            self._add()

    def left(self):  self.dir.rotateLeft()
    def right(self): self.dir.rotateRight()

    def next(self):
        return self.head() + self.dir

    def head(self):
        return self.seg[-1]

    def tick(self):
        self._add()
        if self.growTicks == 0: self._remove()
        if self.growTicks > 0: self.growTicks -= 1

    def _add(self):
        n = self.next()
        if n in self.seg:
            self.kill()
            return
        self.seg.append( n )
        api.world.setBlock(n.x, n.y, n.z, 45)

    def _remove(self):
        h, self.seg = self.seg[0], self.seg[1:]
        api.world.setBlock(h.x, h.y, h.z, 0)

    def grow(self, ticks):
        self.growTicks = ticks

    def kill(self):
        for p in self.seg:
            api.world.setBlock(p.x, p.y, p.z, BlockType.CLOTH, 14)
        self.dead = True

class SnakeGame:
    def __init__(self, numSnakes):
        self.numSnakes = numSnakes
        self.snakes = []
        self.level = None
        self.noActionTicks = 10

    def startLevel(self, level, numApples=8):
        self.level = level
        self.level.setup(numApples)
        self.snakes = [Snake(Vec3(3, 0, 3)) for x in range(self.numSnakes)]
        self.noActionTicks = 10
        for s in self.snakes:
            for t in s.seg: self.level.clearTile(t.x, t.z)
        api.camera.setFixed()
        api.camera.setPos(Vec3((level.w-1)/2.0, 10, level.h/2.0 - 1))

    def tick(self):
        if self.noActionTicks > 0:
            self.noActionTicks -= 1
            return

        for s in self.snakes:
            self.tickSnake(s)

    def tickSnake(self, snake):
        if snake.dead: return
        self.onTile(snake, snake.next())
        if not snake.dead: snake.tick()

    def isFinished(self):
        return self.isWon() or self.allSnakesDead()

    def isWon(self):
        #print self.level.countApples()
        return self.level.countApples() == 0

    def allSnakesDead(self):
        for s in self.snakes:
            if not s.dead: return False
        return True

    def onTile(self, snake, pos):
        x, y = pos.x, pos.z
        if self.level.isWall(x, y): snake.kill()
        if self.level.isFood(x, y): snake.grow(2)
        self.level.clearTile(x, y)

def run():
    kb = kbhelp.KeyboardInput()
    keys = (('a', 'd'),
            ('4', '6'))

    numSnakes = 1

    def play(level, numApples, tickDelay):
        g = SnakeGame(numSnakes)
        g.startLevel(Level(16, 12), numApples)
        api.chat.post("Snake - level %d"%level)

        while 1:
            # Update input
            if kb.hasChar():
                key = kb.readChar()
                for i in range(min(len(g.snakes), len(keys))):
                    LeftKey, RightKey = keys[i]
                    s = g.snakes[i]
                    if key == LeftKey:  s.left()
                    if key == RightKey: s.right()
            # Update game
            g.tick()
            time.sleep(tickDelay)

            if g.isFinished():
                return g.isWon()

    delay = 0.15
    level = 0
    apples = 5

    while 1:
        level  += 1
        apples += 1
        won = play(level, apples, delay)
        if not won:
            api.chat.post("Snake - Game over! [ level %d ]!"%level)
            return

        delay -= 0.01
        if delay < 0.04:
            return


if __name__ == "__main__":
    run()
