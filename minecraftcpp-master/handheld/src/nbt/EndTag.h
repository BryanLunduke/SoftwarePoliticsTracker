#ifndef COM_MOJANG_NBT__EndTag_H__
#define COM_MOJANG_NBT__EndTag_H__

//package com.mojang.nbt;

/* import java.io.* */

class EndTag: public Tag {
	typedef Tag super;
public:
    EndTag()
    :	super("")
	{
    }

    void load(IDataInput* dis) /*throws IOException*/ {
    }

    void write(IDataOutput* dos) /*throws IOException*/ {
    }

    char getId() const {
        return TAG_End;
    }

    std::string toString() const {
        return "END";
    }

    //@Override
    Tag* copy() const {
        return new EndTag();
    }

    //@Override
    bool equals(const Tag& rhs) const {
        return super::equals(rhs);
    }
};

#endif /*COM_MOJANG_NBT__EndTag_H__*/
