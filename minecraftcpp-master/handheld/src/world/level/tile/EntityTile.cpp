#include "EntityTile.h"
#include "entity/TileEntity.h"
#include "../Level.h"
#include "../material/Material.h"

void EntityTile::onPlace( Level* level, int x, int y, int z )
{
	super::onPlace(level, x, y, z);
	level->setTileEntity(x, y, z, newTileEntity());
}

void EntityTile::onRemove( Level* level, int x, int y, int z )
{
	super::onRemove(level, x, y, z);
	level->removeTileEntity(x, y, z);
}

void EntityTile::triggerEvent( Level* level, int x, int y, int z, int b0, int b1 )
{
	super::triggerEvent(level, x, y, z, b0, b1);
	TileEntity* te = level->getTileEntity(x, y, z);
	if (te != NULL) {
		te->triggerEvent(b0, b1);
	}
}

EntityTile::EntityTile( int id, const Material* material )
:	super(id, material)
{
	isEntityTile[this->id] = true;
}

EntityTile::EntityTile( int id, int tex, const Material* material )
:	super(id, tex, material)
{
	isEntityTile[this->id] = true;
}

