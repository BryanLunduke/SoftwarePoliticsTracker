#ifndef COM_MOJANG_NBT__DoubleTag_H__
#define COM_MOJANG_NBT__DoubleTag_H__

//package com.mojang.nbt;

/* import java.io.* */

#include "Tag.h"
#include <sstream>

class DoubleTag: public Tag {
	typedef Tag super;
public:
    double data;

    DoubleTag(const std::string& name)
	:	super(name)
	{}

    DoubleTag(const std::string& name, double data)
	:	super(name),
		data(data)
	{}

    void write(IDataOutput* dos) /*throws IOException*/ {
        dos->writeDouble(data);
    }

    void load(IDataInput* dis) /*throws IOException*/ {
        data = dis->readDouble();
    }

    char getId() const {
        return TAG_Double;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << data;
        return ss.str();
    }

    //@Override
    Tag* copy() const {
        return new DoubleTag(getName(), data);
    }

    //@Override
    bool equals(const Tag& rhs) const {
        if (super::equals(rhs)) {
            return data == ((DoubleTag&)rhs).data;
        }
        return false;
    }
};

#endif /*COM_MOJANG_NBT__DoubleTag_H__*/
