#ifndef COM_MOJANG_NBT__ByteArrayTag_H__
#define COM_MOJANG_NBT__ByteArrayTag_H__

//package com.mojang.nbt;

/* import java.io.* */

#include "Tag.h"
#include <string>

typedef struct TagMemoryChunk {
	TagMemoryChunk()
	:	data(NULL),
		len(0)
	{}
	void* data;
	int len;
} TagMemoryChunk;

class ByteArrayTag: public Tag
{
	typedef Tag super;
public:
    TagMemoryChunk data;

    ByteArrayTag(const std::string& name)
    :   super(name)
	{
    }

    ByteArrayTag(const std::string& name, TagMemoryChunk data)
    :   super(name),
		data(data)
	{
    }

    char getId() const {
        return TAG_Byte_Array;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << "[" << data.len << " bytes]";
        return ss.str();
    }

    //@Override
    bool equals(const Tag& rhs) const {
        if (super::equals(rhs)) {
			ByteArrayTag& o = (ByteArrayTag&) rhs;
			if (data.len != o.data.len)
				return false;

			return memcmp(data.data, o.data.data, data.len) == 0;
        }
        return false;
    }

    //@Override
    Tag* copy() const {
		TagMemoryChunk chunk;
        chunk.data = new char[data.len];
		memcpy(chunk.data, data.data, data.len);
        return new ByteArrayTag(getName(), chunk);
    }

    void write(IDataOutput* dos) /*throws IOException*/ {
        dos->writeInt(data.len);
        dos->writeBytes(data.data, data.len);
    }

    void load(IDataInput* dis) /*throws IOException*/ {
        int length = dis->readInt();
        data.data = new char[length];
        dis->readBytes(data.data, length);
    }
};

#endif /*COM_MOJANG_NBT__ByteArrayTag_H__*/
