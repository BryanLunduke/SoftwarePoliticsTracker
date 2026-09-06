#ifndef COM_MOJANG_NBT__FloatTag_H__
#define COM_MOJANG_NBT__FloatTag_H__

//package com.mojang.nbt;

/* import java.io.* */

#include "Tag.h"
#include <sstream>

class FloatTag: public Tag {
	typedef Tag super;
public:
    float data;

    FloatTag(const std::string& name)
	:	super(name)
	{
    }

    FloatTag(const std::string& name, float data)
	:	super(name),
		data(data)
	{
    }

    void write(IDataOutput* dos) /*throws IOException*/ {
        dos->writeFloat(data);
    }

    void load(IDataInput* dis) /*throws IOException*/ {
        data = dis->readFloat();
    }

    char getId() const {
        return TAG_Float;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << data;
        return ss.str();
    }

    //@Override
    Tag* copy() const {
        return new FloatTag(getName(), data);
    }

    //@Override
    bool equals(const Tag& rhs) const {
        if (super::equals(rhs)) {
            return data == ((FloatTag&)rhs).data;
        }
        return false;
    }
};

#endif /*COM_MOJANG_NBT__FloatTag_H__*/
