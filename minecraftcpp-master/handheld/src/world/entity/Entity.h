#ifndef NET_MINECRAFT_WORLD_ENTITY__Entity_H__
#define NET_MINECRAFT_WORLD_ENTITY__Entity_H__

//package net.minecraft.world.entity;

class Level;
class Player;
class EntityPos;
class Material;
class ItemEntity;
class ItemInstance;
class CompoundTag;

#include "EntityRendererId.h"
#include "../phys/AABB.h"
#include "../../SharedConstants.h"
//#include "../../util/MemUtils.h"
#include "../../util/Mth.h"
#include "../../util/Random.h"

class SynchedEntityData;

class Entity
{
public:
	static int entityCounter;
	static const int TOTAL_AIR_SUPPLY = 15 * SharedConstants::TicksPerSecond;

    Entity(Level* level);
	virtual ~Entity();

	void _init();
	virtual void reset(); // { super::reset(); _init(); }

	int hashCode();
	bool operator==(Entity& rhs);

	virtual void setLevel(Level* level);
    
	virtual void remove();

    virtual void setPos(float x, float y, float z);
	virtual void move(float xa, float ya, float za);
	virtual void moveTo(float x, float y, float z, float yRot, float xRot);
	virtual void moveRelative(float xa, float za, float speed);

	virtual void lerpTo(float x, float y, float z, float yRot, float xRot, int steps);
	virtual void lerpMotion(float xd, float yd, float zd);

    virtual void turn(float xo, float yo);
    virtual void interpolateTurn(float xo, float yo);

    virtual void tick();
	virtual void baseTick();

	virtual bool intersects(float x0, float y0, float z0, float x1, float y1, float z1);
    virtual bool isFree(float xa, float ya, float za, float grow);
    virtual bool isFree(float xa, float ya, float za);
	virtual bool isInWall();
    virtual bool isInWater();
	virtual bool isInLava();
    virtual bool isUnderLiquid(const Material* material);

	virtual void makeStuckInWeb();

	virtual float getHeadHeight();
	virtual float getShadowHeightOffs();

    virtual float getBrightness(float a);

    float distanceTo(Entity* e);
    float distanceTo(float x2, float y2, float z2);
    float distanceToSqr(float x2, float y2, float z2);
    float distanceToSqr(Entity* e);

	virtual bool interactPreventDefault();
	virtual bool interact(Player* player);
    virtual void playerTouch(Player* player);

    virtual void push(Entity* e);
    virtual void push(float xa, float ya, float za);
    
    virtual bool isPickable();
    virtual bool isPushable();
    virtual bool isShootable();

	virtual bool isSneaking();

	virtual bool isAlive();
	virtual bool isOnFire();

	virtual bool isPlayer();
	virtual bool isCreativeModeAllowed();

    virtual bool shouldRender(Vec3& c);
    virtual bool shouldRenderAtSqrDistance(float distance);

	virtual bool hurt(Entity* source, int damage);
    virtual void animateHurt();

    virtual void handleEntityEvent(char eventId) {}

 	virtual float getPickRadius();

	virtual ItemEntity* spawnAtLocation(int resource, int count);
	virtual ItemEntity* spawnAtLocation(int resource, int count, float yOffs);
	// @attn: right now this means a pointer that spawnAtLocation takes ownership of
	virtual ItemEntity* spawnAtLocation(ItemInstance* itemInstance, float yOffs);

	virtual void awardKillScore(Entity* victim, int score);

	virtual void setEquippedSlot(int slot, int item, int auxValue);

	virtual bool save(CompoundTag* entityTag);
	virtual void saveWithoutId(CompoundTag* entityTag);
	virtual bool load(CompoundTag* entityTag);
	virtual SynchedEntityData* getEntityData();
	virtual const SynchedEntityData* getEntityData() const;

	__inline bool isEntityType(int type) { return getEntityTypeId() == type; }
	virtual int getEntityTypeId() const = 0;
	virtual int getCreatureBaseType() const { return 0; }
	virtual EntityRendererId queryEntityRenderer() { return ER_DEFAULT_RENDERER; }

	virtual bool isMob() { return false; }
	// I hate myself
	virtual bool isItemEntity();
	// Me 2
	virtual bool isHangingEntity();

	virtual int getAuxData();

protected:
	virtual void setRot(float yRot, float xRot);
	virtual void setSize(float w, float h);
	virtual void setPos(EntityPos* pos);
	virtual void resetPos(bool clearMore);
	virtual void outOfWorld();

	virtual void checkFallDamage(float ya, bool onGround);
	virtual void causeFallDamage(float fallDamage2);
	virtual void markHurt();

	virtual void burn(int dmg);
	virtual void lavaHurt();

	virtual void readAdditionalSaveData(CompoundTag* tag) = 0;
	virtual void addAdditonalSaveData(CompoundTag* tag) = 0;

    virtual void playStepSound( int xt, int yt, int zt, int t );
public:
	float x, y, z;

	int xChunk, yChunk, zChunk;

	int entityId;

	float viewScale;

	Level* level;
	float xo, yo, zo;
	float xd, yd, zd;
	float yRot, xRot;
	float yRotO, xRotO;

	AABB bb;

	float heightOffset; // = 0 / 16.0f;

	float bbWidth;
	float bbHeight;

	float walkDistO;
	float walkDist;

	float xOld, yOld, zOld;
	float ySlideOffset;
	float footSize;
	float pushthrough;

	int tickCount;
	int invulnerableTime;
	int airSupply;
	int onFire;
	int flameTime;

	EntityRendererId entityRendererId;

	//Ref<Entity>* ref;

	//   bool hovered = false;

	//   std::string customTextureUrl;
	//   std::string customTextureUrl2;
	//   /*protected*/ bool fireImmune = false;

	float fallDistance;
	bool blocksBuilding;
	bool inChunk;

	bool onGround;
	bool horizontalCollision, verticalCollision;
	bool collision;
	bool hurtMarked;

	bool slide;
	bool removed;
	bool noPhysics;
	bool canRemove;
	bool invisible;
	bool reallyRemoveIfPlayer;
protected:
	static Random sharedRandom;
	int airCapacity;
	bool makeStepSound;
	bool wasInWater;
	bool fireImmune;

protected:
	bool firstTick;
	int nextStep;
	static const int DATA_AIR_SUPPLY_ID = 1;
	bool isStuckInWeb;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY__Entity_H__*/
