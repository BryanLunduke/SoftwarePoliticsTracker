from vec3 import Vec3

class TileEvent:
    HIT = 0

    def __init__(self, type, x, y, z, face, entityId):
        self.type = type
        self.pos = Vec3(x, y, z)
        self.face = face
        self.entityId = entityId

    def __str__(self):
        types = {TileEvent.HIT: "Hit"}
        return "TileEvent(%s @ %s:%d by %d)"%(types.get(self.type, 0),
            self.pos, self.face, self.entityId);

    @staticmethod
    def Hit(x, y, z, face, entityId):
        return TileEvent(TileEvent.HIT, x, y, z, face, entityId)
