#include "SignTileEntity.h"
#include "../../../../network/packet/SignUpdatePacket.h"
#include "../../Level.h"
SignTileEntity::SignTileEntity()
:   super(TileEntityType::Sign),
	selectedLine(-1),
	editable(true)
{
    rendererId = TR_SIGN_RENDERER;
}

bool SignTileEntity::save( CompoundTag* tag ) {
	if (!super::save(tag))
		return false;

	tag->putString("Text1", messages[0]);
	tag->putString("Text2", messages[1]);
	tag->putString("Text3", messages[2]);
	tag->putString("Text4", messages[3]);
	return true;
}

void SignTileEntity::load( CompoundTag* tag ) {
	editable = false;
	super::load(tag);

	messages[0] = tag->getString("Text1");
	messages[1] = tag->getString("Text2");
	messages[2] = tag->getString("Text3");
	messages[3] = tag->getString("Text4");

	for (int i = 0; i < NUM_LINES; i++)
		if (messages[i].length() > MAX_LINE_LENGTH)
			messages[i].resize(MAX_LINE_LENGTH);
}

bool SignTileEntity::shouldSave() {
    return true;
}

bool SignTileEntity::isEditable() {
	return editable;
}

void SignTileEntity::setEditable( bool isEditable ) {
	this->editable = isEditable;
}

Packet* SignTileEntity::getUpdatePacket() {
	return new SignUpdatePacket(x, y, z, messages);
}

void SignTileEntity::setLevelAndPos( Level* level, int x, int y, int z ) {
	super::setLevelAndPos(level, x, y, z);
	if(level->getTile(x, y, z) != Tile::sign->id) {
		remove = true;
	}
}
