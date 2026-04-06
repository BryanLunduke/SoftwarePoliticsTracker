#ifndef NET_MINECRAFT_WORLD_ENTITY_ITEM__TripodCamera_H__
#define NET_MINECRAFT_WORLD_ENTITY_ITEM__TripodCamera_H__

#include "../Mob.h"

class TripodCamera: public Mob
{
	typedef Mob super;
public:
    TripodCamera(Level* level, Player* owner_, float x, float y, float z);

	void tick();

	bool isPickable();
	bool isPushable();

	// id == 0 -> not possible to create via serialization (yet)
	int getEntityTypeId() const { return 0; }

	bool interact(Player* player);
	bool interactPreventDefault();

    float getShadowHeightOffs();
public:
	int life;
protected:
	Player* owner;
	bool activated;
};


#endif /*NET_MINECRAFT_WORLD_ENTITY_ITEM__TripodCamera_H__*/
