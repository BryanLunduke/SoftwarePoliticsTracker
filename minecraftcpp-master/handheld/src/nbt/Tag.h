#ifndef COM_MOJANG_NBT__Tag_H__
#define COM_MOJANG_NBT__Tag_H__

//package com.mojang.nbt;

/* import java.io.* */

#include "../util/DataIO.h"

#include <string>


class Tag
{
public:
    virtual ~Tag() {}
	virtual void deleteChildren() {}

    static const char TAG_End		= 0;
    static const char TAG_Byte		= 1;
    static const char TAG_Short		= 2;
    static const char TAG_Int		= 3;
    static const char TAG_Long		= 4;
    static const char TAG_Float		= 5;
    static const char TAG_Double	= 6;
    static const char TAG_Byte_Array= 7;
    static const char TAG_String	= 8;
    static const char TAG_List		= 9;
    static const char TAG_Compound	= 10;

	static const std::string NullString;

    virtual void write(IDataOutput* dos) = 0; ///*throws IOException*/;
    virtual void load(IDataInput* dis) = 0; ///*throws IOException*/;

    virtual std::string toString() const = 0;
    virtual char getId() const = 0;

    virtual bool equals(const Tag& rhs) const;

    virtual void print(PrintStream& out) const;
    virtual void print(const std::string& prefix, PrintStream& out) const;

    virtual Tag* setName(const std::string& name);
    virtual std::string getName() const;

    static Tag* readNamedTag(IDataInput* dis);
    static void writeNamedTag(Tag* tag, IDataOutput* dos);

    static Tag* newTag(char type, const std::string& name);

    static std::string getTagName(char type);

	virtual Tag* copy() const = 0;

	mutable int errorState;
	static const int TAGERR_OUT_OF_BOUNDS = 1;
	static const int TAGERR_BAD_TYPE = 2;
protected:
    Tag(const std::string& name);
private:
    std::string name;
};

#endif /*COM_MOJANG_NBT__Tag_H__*/
