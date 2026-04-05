#ifndef NET_MINECRAFT_WORLD_ENTITY__EntityFactory_H__
#define NET_MINECRAFT_WORLD_ENTITY__EntityFactory_H__

class Level;
class Entity;
class CompoundTag;

class EntityFactory
{
public:
	static Entity* CreateEntity(int typeId, Level* level);
	static Entity* loadEntity(CompoundTag* tag, Level* level);
};

#endif /*NET_MINECRAFT_WORLD_ENTITY__EntityFactory_H__*/
