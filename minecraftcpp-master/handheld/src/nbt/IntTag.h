#ifndef COM_MOJANG_NBT__IntTag_H__
#define COM_MOJANG_NBT__IntTag_H__

//package com.mojang.nbt;

#include "Tag.h"
#include <sstream>
/* import java.io.* */

class IntTag: public Tag {
	typedef Tag super;
public:
    int data;

    IntTag(const std::string& name)
	:	super(name)
	{
    }

    IntTag(const std::string& name, int data)
	:	super(name),
		data(data)
	{
    }

    void write(IDataOutput* dos) /*throws IOException*/ {
        dos->writeInt(data);
    }

    void load(IDataInput* dis) /*throws IOException*/ {
        data = dis->readInt();
    }

    char getId() const {
        return TAG_Int;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << data;
        return ss.str();
    }

    //@Override
    Tag* copy() const {
        return new IntTag(getName(), data);
    }

    //@Override
    bool equals(const Tag& rhs) const {
        if (super::equals(rhs)) {
            return data == ((IntTag&)rhs).data;
        }
        return false;
    }
};

#endif /*COM_MOJANG_NBT__IntTag_H__*/
