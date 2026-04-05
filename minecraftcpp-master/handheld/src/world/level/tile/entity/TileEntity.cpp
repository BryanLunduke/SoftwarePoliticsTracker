#include "TileEntity.h"

TileEntity::MapIdType TileEntity::idClassMap;
TileEntity::MapTypeId TileEntity::classIdMap;

#include "FurnaceTileEntity.h"
#include "ChestTileEntity.h"
#include "NetherReactorTileEntity.h"
#include "../../Level.h"
#include "../../../../nbt/CompoundTag.h"
#include "SignTileEntity.h"

int TileEntity::_runningId = 0;

//
// TileEntityFactory
//
TileEntity* TileEntityFactory::createTileEntity( int type )
{
	switch(type) {
	case TileEntityType::Furnace:   return new FurnaceTileEntity();
	case TileEntityType::Chest:	    return new ChestTileEntity();
    case TileEntityType::Sign:		return new SignTileEntity();
    case TileEntityType::NetherReactor:	return new NetherReactorTileEntity();
	default:
		LOGE("Can't find TileEntity of type: %d\n", type);
		return NULL;
	}
}

//
// TileEntity
//

void TileEntity::initTileEntities()
{
	setId(TileEntityType::Furnace, "Furnace");
	setId(TileEntityType::Chest,   "Chest");
	setId(TileEntityType::NetherReactor, "NetherReactor");
	//         setId(ChestTileEntity.class, "Chest");
	//         setId(RecordPlayerTile.Entity.class, "RecordPlayer");
	//         setId(DispenserTileEntity.class, "Trap");
	setId(TileEntityType::Sign, "Sign");
	//         setId(MobSpawnerTileEntity.class, "MobSpawner");
	//         setId(MusicTileEntity.class, "Music");
	//         setId(PistonPieceEntity.class, "Piston");
	//         setId(BrewingStandTileEntity.class, "Cauldron");
	//         setId(EnchantmentTableEntity.class, "EnchantTable");
	//         setId(TheEndPortalTileEntity.class, "Airportal");
}

void TileEntity::teardownTileEntities()
{

}

TileEntity::TileEntity( int tileEntityType )
:	data(-1),
	type(tileEntityType),
	remove(false),
	level(NULL),
	tile(NULL),
	clientSideOnly(false),
	rendererId(TR_DEFAULT_RENDERER),
	runningId(++_runningId)
{
}

void TileEntity::load( CompoundTag* tag )
{
	x = tag->getInt("x");
	y = tag->getInt("y");
	z = tag->getInt("z");
	LOGI("Loaded tile entity @ %d, %d, %d\n", x, y, z);
}

bool TileEntity::save( CompoundTag* tag )
{
	MapTypeId::const_iterator it = classIdMap.find(type);
	if (it == classIdMap.end()) {
		LOGE("TileEntityType %d is missing a mapping! This is a bug!\n", type);
		return false;
	}
	tag->putString("id", it->second);
	tag->putInt("x", x);
	tag->putInt("y", y);
	tag->putInt("z", z);
	return true;
}

TileEntity* TileEntity::loadStatic( CompoundTag* tag )
{
	TileEntity* entity = NULL;

	std::string id = tag->getString("id");
	MapIdType::const_iterator cit = idClassMap.find(id);
	if (cit != idClassMap.end()) {
		//LOGI("Loading Tile Entity @ %d,%d,%d\n", ));
		entity = TileEntityFactory::createTileEntity(cit->second);
		if (entity) {
			//LOGI("Loading TileEntity: %d\n", entity->type);
			entity->load(tag);
		}
		else
			LOGE("Couldn't create TileEntity of type: '%d'\n", cit->second);
	} else {
		LOGE("Couldn't find TileEntity type: '%s'\n", id.c_str());
	}
	return entity;
}

int TileEntity::getData()
{
	if (data == -1) data = level->getData(x, y, z);
	return data;
}

void TileEntity::setData( int data )
{
	this->data = data;
	level->setData(x, y, z, data);
}

void TileEntity::setChanged()
{
	if (level != NULL) {
		data = level->getData(x, y, z);
		level->tileEntityChanged(x, y, z, this);
	}
}

float TileEntity::distanceToSqr( float xPlayer, float yPlayer, float zPlayer )
{
	float xd = (x + 0.5f) - xPlayer;
	float yd = (y + 0.5f) - yPlayer;
	float zd = (z + 0.5f) - zPlayer;
	return xd * xd + yd * yd + zd * zd;
}

Tile* TileEntity::getTile() {
	if (tile == NULL) tile = Tile::tiles[level->getTile(x, y, z)];
	return tile;
}

bool TileEntity::isRemoved() const
{
	return remove;
}

void TileEntity::setRemoved()
{
	remove = true;
}

void TileEntity::clearRemoved()
{
	remove = false;
}

void TileEntity::triggerEvent( int b0, int b1 )
{
}

void TileEntity::clearCache()
{
	tile = NULL;
	data = -1;
}

void TileEntity::setId( int type, const std::string& id )
{
	MapIdType::const_iterator cit = idClassMap.find(id);
	if (cit != idClassMap.end()) {
		LOGE("Duplicate id: %s\n", id.c_str());
		return;
	}

	idClassMap.insert(std::make_pair(id, type));
	classIdMap.insert(std::make_pair(type, id));
}

void TileEntity::setLevelAndPos( Level* level, int x, int y, int z ) {
	this->level = level;
	this->x = x;
	this->y = y;
	this->z = z;

	if (level) {
		this->tile = Tile::tiles[level->getTile(x, y, z)];
	}
}

bool TileEntity::isFinished() {
	return false;
}

Packet* TileEntity::getUpdatePacket() {
	return NULL;
}

bool TileEntity::isType( int Type ) {
	return type == Type;
}

bool TileEntity::isType( TileEntity* te, int Type ) {
	return te != NULL && te->isType(Type);
}


int partitionTileEntities( const std::vector<TileEntity*>& in, std::vector<TileEntity*>& keep, std::vector<TileEntity*>& dontKeep ) {
	std::map<TilePos, TileEntity*> m;
	for (unsigned int i = 0; i < in.size(); ++i) {
		TileEntity* e = in[i];
		TilePos p(e->x, e->y, e->z);
		if (m.find(p) == m.end() && e->shouldSave()) {
			m.insert(std::make_pair(p, e));
			keep.push_back(e);
		} else dontKeep.push_back(e);
	}
	return (int)keep.size();
}
