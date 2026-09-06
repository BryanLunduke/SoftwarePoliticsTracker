#ifndef NET_MINECRAFT_WORLD_ENTITY__SynchedEntityData_H__
#define NET_MINECRAFT_WORLD_ENTITY__SynchedEntityData_H__

//package net.minecraft.world.entity;

#include "../Pos.h"
#include "../../network/Packet.h"
#include "../item/ItemInstance.h"
#include "../item/Item.h"
#include "../../util/DataIO.h"

class DataItem {
public:
	DataItem(int type, int id)
	:	type(type),
		id(id),
		dirty(true)
	{}
	virtual ~DataItem() {}

	int getId() const {
		return id;
	}

	int getType() const {
		return type;
	}

	bool isDirty() const {
		return dirty;
	}

	void setDirty(bool dirty) {
		this->dirty = dirty;
	}
	virtual bool isDataEqual(const DataItem& rhs) const {
		return type == rhs.type;
	}
private:
	const int type;
	const int id;
	bool dirty;
/*
public:
	// attn: Very un-typesafe, but let's go
	union {
		char         _char;
		short        _short;
		int          _int;
		float        _float;
		std::string  _String;
		ItemInstance _ItemInstance;
		Pos          _Pos;
	};
	*/
};

template <class T>
class DataItem2: public DataItem {
	typedef DataItem super;
public:
	DataItem2(int type, int id, const T& data)
	:	super(type, id),
		data(data)
	{}
	void setFlag(int flag) {
		data = data | (1 << flag);
	}
	void clearFlag(int flag) {
		data = data & ~(1 << flag);
	}
	bool getFlag(int flag) const {
		return (data & (1 << flag)) != 0;
	}
	T data;
};

/*
#define DataItemClassFactory2(Postfix, TypeName)		\
class DataItem_ ## Postfix : public DataItem {				\
	typedef DataItem super;								\
public:													\
	DataItem_ ## Postfix(int type, int id, #Typename data)	\
	:	super(type),									\
		data(data)										\
	{}													\
	virtual bool isDataEqual(const DataItem& rhs) const {		\
		if (!super::isDataEqual(rhs)) return;			\
		return data == (DataItem_ ## #Postfix&)rhs.data;	\
	}													\
	#Typename data;										\
};
#define DataItemClassFactory(TypeName)	DataItemClassFactory2(TypeName, TypeName)

DataItemClassFactory(char);
DataItemClassFactory(short);
DataItemClassFactory(int);
DataItemClassFactory(float);
DataItemClassFactory(ItemInstance);
DataItemClassFactory(Pos);
DataItemClassFactory2(String, std::string);
*/

class SynchedEntityData {
public:
	static const int MAX_STRING_DATA_LENGTH = 64;
	static const int EOF_MARKER = 0x7f;

	typedef signed char TypeChar;
	typedef short		TypeShort;
	typedef int			TypeInt;
	typedef float		TypeFloat;
private:
    static const int TYPE_BYTE = 0;
    static const int TYPE_SHORT = 1;
    static const int TYPE_INT = 2;
    static const int TYPE_FLOAT = 3;
    static const int TYPE_STRING = 4;
    // special types (max possible value is 7):
    static const int TYPE_ITEMINSTANCE = 5;
    static const int TYPE_POS = 6;

    // must have enough bits to fit the type
    static const int TYPE_MASK = 0xe0;
    static const int TYPE_SHIFT = 5;

    // the id value must fit in the remaining bits
    static const int MAX_ID_VALUE = ~TYPE_MASK & 0xff;

	typedef std::map<int, DataItem*> Map;
public:
	typedef std::vector<DataItem*> DataList;

    SynchedEntityData();

	~SynchedEntityData();

    int getTypeId(const TypeChar &)  { return TYPE_BYTE; }
    int getTypeId(const TypeShort&)  { return TYPE_SHORT; }
    int getTypeId(const TypeInt&)    { return TYPE_INT; }
    int getTypeId(const TypeFloat&)  { return TYPE_FLOAT; }
    int getTypeId(const std::string&){ return TYPE_STRING; }
    int getTypeId(const ItemInstance&){return TYPE_ITEMINSTANCE; }
    int getTypeId(const Pos&)        { return TYPE_POS; }

	template <class T>
    void define(int idd, const T& value) {
        int type = getTypeId(value);

        if (idd > MAX_ID_VALUE) {
            LOGE("Data value id is too big with %d! (Max is %d )", idd, MAX_ID_VALUE);
            return;
        }

        if (itemsById.find(idd) != itemsById.end()) {
            LOGE("Duplicate id value for %d!\n", idd);
            return;
        }

        DataItem* dataItem = new DataItem2<T>(type, idd, value);
        itemsById.insert(std::make_pair(idd, dataItem));
    }

    TypeChar      getByte(int id) const;
    TypeShort     getShort(int id) const;
    TypeInt       getInt(int id) const;
    TypeFloat     getFloat(int id) const;
    std::string   getString(int id) const;
    ItemInstance  getItemInstance(int id);
    Pos           getPos(int id) const;

	template <class T>
    void set(int id, const T& value) {
        set( itemsById[id], value );
    }
	template <class T>
	void set(DataItem* dataItem, const T& value) {
		if (!dataItem) {
			LOGE("DataItem not found!\n");
			return;
		}
		if (dataItem->getType() == getTypeId(value)) {
			DataItem2<T>* d = (DataItem2<T>*)dataItem;
			if (!(d->data == value)) {
				d->data = value;
				dataItem->setDirty(true);
				_isDirty = true;
			}
		} else {
			LOGE("ERROR: Id %d is not correct type. %d != %d!\n", dataItem->getId(), dataItem->getType(), getTypeId(value));
		}
	}
	/// Set flag bit. Flag is bit shifted.
	template <class T>
	void setFlag(int id, int flag) {
		DataItem2<T>* item = (DataItem2<T>*)itemsById[id];
		bool oldFlag = item->getFlag(flag);
		item->setFlag(flag);
		if(item->getFlag(flag) != oldFlag) {
			markDirty(id);
		}
	}
	/// Clear flag bit. Flag is bit shifted.
	template <class T>
	void clearFlag(int id, int flag) {
		DataItem2<T>* item = (DataItem2<T>*)itemsById[id];
		bool oldFlag = item->getFlag(flag);
		item->clearFlag(flag);
		if(item->getFlag(flag) != oldFlag) {
			markDirty(id);
		}
	}
	/// Get flag bit. Flag is bit shifted.
	template <class T>
	bool getFlag(int id, int flag) {
		const DataItem2<T>* item = (const DataItem2<T>*)itemsById[id];
		return item->getFlag(flag);
	}

    void markDirty(int id) {
        itemsById[id]->setDirty(true);
        _isDirty = true;
    }

    bool isDirty() const {
        return _isDirty;
    }

    static void pack(DataList* items, IDataOutput* output);

    DataList packDirty();

    void packAll(IDataOutput* output) const;

    static DataList unpack(IDataInput* input);

    /**
     * Assigns values from a list of data items.
     * 
     * @param items
     */
	void assignValues(DataList* items);

private:
    static void writeDataItem(IDataOutput* output, const DataItem* dataItem);

    Map itemsById;
    bool _isDirty;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY__SynchedEntityData_H__*/
