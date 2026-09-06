#include "SynchedEntityData.h"


SynchedEntityData::SynchedEntityData()
:	_isDirty(false)
{

}

SynchedEntityData::~SynchedEntityData()
{
	for (Map::iterator it = itemsById.begin(); it != itemsById.end(); ++it) {
		delete (it->second);
	}
}

void SynchedEntityData::pack( DataList* items, IDataOutput* output )
{
	if (items != NULL) {
		for (DataList::iterator it = items->begin(); it != items->end(); ++it)
			writeDataItem(output, *it);
	}
	// add an eof
	output->writeByte(EOF_MARKER);
}

SynchedEntityData::DataList SynchedEntityData::packDirty()
{
	DataList result;
	if (_isDirty) {
		for (Map::iterator it = itemsById.begin(); it != itemsById.end(); ++it) {
			DataItem* dataItem = it->second;
			if (dataItem->isDirty()) {
				dataItem->setDirty(false);
				result.push_back(dataItem);
			}
		}
	}
	_isDirty = false;

	return result;
}

void SynchedEntityData::packAll( IDataOutput* output ) const
{
	for (Map::const_iterator cit = itemsById.begin(); cit != itemsById.end(); ++cit)
		SynchedEntityData::writeDataItem(output, cit->second);

	// add an eof
	output->writeByte(EOF_MARKER);
}

SynchedEntityData::DataList SynchedEntityData::unpack( IDataInput* input )
{
	DataList result;

	int currentHeader = input->readByte();

	while (currentHeader != EOF_MARKER) {
		// split type and id
		int itemType = (currentHeader & TYPE_MASK) >> TYPE_SHIFT;
		int itemId = (currentHeader & MAX_ID_VALUE);

		//LOGI("unpacking: %d, %d\n", itemId, itemType);

		DataItem* item = NULL;
		switch (itemType) {
		case TYPE_BYTE:
			item = new DataItem2<TypeChar>(itemType, itemId, input->readByte());
			break;
		case TYPE_SHORT:
			item = new DataItem2<TypeShort>(itemType, itemId, input->readShort());
			break;
		case TYPE_INT:
			item = new DataItem2<TypeInt>(itemType, itemId, input->readInt());
			break;
		case TYPE_FLOAT:
			item = new DataItem2<TypeFloat>(itemType, itemId, input->readFloat());
			break;
		case TYPE_STRING:
			item = new DataItem2<std::string>(itemType, itemId, input->readString());
			break;
		case TYPE_ITEMINSTANCE: {
			int id = input->readShort();
			int count = input->readByte();
			int auxValue = input->readShort();
			item = new DataItem2<ItemInstance>(itemType, itemId, ItemInstance(id, count, auxValue));
			}
			break;
		case TYPE_POS: {
			int x = input->readInt();
			int y = input->readInt();
			int z = input->readInt();
			item = new DataItem2<Pos>(itemType, itemId, Pos(x, y, z));
			}
			break;
		}
		result.push_back(item);

		currentHeader = input->readByte();
	}

	return result;
}

void SynchedEntityData::assignValues( DataList* items )
{
	for (DataList::const_iterator it = items->begin(); it != items->end(); ++it) {

		//for (DataItem item : items) {
		DataItem* item = *it;
		Map::iterator jt = itemsById.find(item->getId());
		//DataItem dataItem = itemsById.get(item.getId());
		if (jt != itemsById.end()) {
			switch (item->getType()) {
			case TYPE_BYTE : set(jt->second, ((DataItem2<TypeChar>*)item)->data); break;
			case TYPE_SHORT: set(jt->second, ((DataItem2<TypeShort>*)item)->data); break;
			case TYPE_INT  : set(jt->second, ((DataItem2<TypeInt>*)item)->data); break;
			case TYPE_FLOAT: set(jt->second, ((DataItem2<TypeFloat>*)item)->data); break;
			case TYPE_STRING:set(jt->second, ((DataItem2<std::string>*)item)->data); break;
			case TYPE_ITEMINSTANCE: set(jt->second, ((DataItem2<ItemInstance>*)item)->data); break;
			case TYPE_POS:   set(jt->second, ((DataItem2<Pos>*)item)->data); break;
			default:
				LOGE("Incorrect type id: %d\n", item->getType());
				break;
			}
			//dataItem->setValue(item->getValue());
		}
	}
}

void SynchedEntityData::writeDataItem( IDataOutput* output, const DataItem* dataItem )
{
	//LOGI("write: %d, %d\n", dataItem->getId(), dataItem->getType());

	// pack type and id
	//LOGI("dataItem: %d\n", dataItem);
	int header = ((dataItem->getType() << TYPE_SHIFT) | (dataItem->getId() & MAX_ID_VALUE)) & 0xff;
	output->writeByte(header);

	// write value
	switch (dataItem->getType()) {
	case TYPE_BYTE:
		output->writeByte(((DataItem2<TypeChar>*)dataItem)->data);
		break;
	case TYPE_SHORT:
		output->writeShort(((DataItem2<TypeShort>*)dataItem)->data);
		break;
	case TYPE_INT:
		output->writeInt(((DataItem2<TypeInt>*)dataItem)->data);
		break;
	case TYPE_FLOAT:
		output->writeFloat(((DataItem2<TypeFloat>*)dataItem)->data);
		break;
	case TYPE_STRING:
		output->writeString(((DataItem2<std::string>*)dataItem)->data);
		break;
	case TYPE_ITEMINSTANCE: {
		const ItemInstance& instance = ((DataItem2<ItemInstance>*)dataItem)->data;
		output->writeShort(instance.getItem()->id);
		output->writeByte(instance.count);
		output->writeShort(instance.getAuxValue());
		}
		break;
	case TYPE_POS: {
		const Pos& instance = ((DataItem2<Pos>*)dataItem)->data;
		output->writeInt(instance.x);
		output->writeInt(instance.y);
		output->writeInt(instance.z);
		}
		break;
	}
}

SynchedEntityData::TypeChar SynchedEntityData::getByte( int id ) const
{
	Map::const_iterator it = itemsById.find(id);
	if (it != itemsById.end()) {
		if (it->second->getType() == TYPE_BYTE) {
			return ((DataItem2<TypeChar>*)it->second)->data;
		}
	}
	return 0;
}

SynchedEntityData::TypeShort SynchedEntityData::getShort( int id ) const
{
	Map::const_iterator it = itemsById.find(id);
	if (it != itemsById.end()) {
		if (it->second->getType() == TYPE_SHORT) {
			return ((DataItem2<TypeShort>*)it->second)->data;
		}
	}
	return 0;
}

SynchedEntityData::TypeInt SynchedEntityData::getInt( int id ) const
{
	Map::const_iterator it = itemsById.find(id);
	if (it != itemsById.end()) {
		if (it->second->getType() == TYPE_INT) {
			return ((DataItem2<TypeInt>*)it->second)->data;
		}
	}
	return 0;
}

SynchedEntityData::TypeFloat SynchedEntityData::getFloat( int id ) const
{
	Map::const_iterator it = itemsById.find(id);
	if (it != itemsById.end()) {
		if (it->second->getType() == TYPE_FLOAT) {
			return ((DataItem2<TypeFloat>*)it->second)->data;
		}
	}
	return 0;
}

std::string SynchedEntityData::getString( int id ) const
{
	Map::const_iterator it = itemsById.find(id);
	if (it != itemsById.end()) {
		if (it->second->getType() == TYPE_STRING) {
			return ((DataItem2<std::string>*)it->second)->data;
		}
	}
	return "";
}

ItemInstance SynchedEntityData::getItemInstance( int id )
{
	Map::const_iterator it = itemsById.find(id);
	if (it != itemsById.end()) {
		if (it->second->getType() == TYPE_ITEMINSTANCE) {
			return ((DataItem2<ItemInstance>*)it->second)->data;
		}
	}
	return ItemInstance();
}

Pos SynchedEntityData::getPos( int id ) const
{
	Map::const_iterator it = itemsById.find(id);
	if (it != itemsById.end()) {
		if (it->second->getType() == TYPE_POS) {
			return ((DataItem2<Pos>*)it->second)->data;
		}
	}
	return Pos();
}
