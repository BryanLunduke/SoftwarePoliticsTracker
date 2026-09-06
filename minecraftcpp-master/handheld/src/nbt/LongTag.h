#ifndef COM_MOJANG_NBT__LongTag_H__
#define COM_MOJANG_NBT__LongTag_H__

//package com.mojang.nbt;
#include "Tag.h"
#include <sstream>

class LongTag: public Tag {
	typedef Tag super;
public:
    long long data;

    LongTag(const std::string& name)
    :   super(name)
	{
    }

    LongTag(const std::string& name, long long data)
    :   super(name),
		data(data)
	{
    }

    void write(IDataOutput* dos) /*throws IOException*/ {
        dos->writeLongLong(data);
    }

    void load(IDataInput* dis) /*throws IOException*/ {
        data = dis->readLongLong();
    }

    char getId() const {
        return TAG_Long;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << data;
        return ss.str();
    }

    //@Override
    Tag* copy() const {
        return new LongTag(getName(), data);
    }

    //@Override
    bool equals(const Tag& rhs) const {
        if (super::equals(rhs)) {
            return data == ((LongTag&)rhs).data;
        }
        return false;
    }
};

#endif /*COM_MOJANG_NBT__LongTag_H__*/
