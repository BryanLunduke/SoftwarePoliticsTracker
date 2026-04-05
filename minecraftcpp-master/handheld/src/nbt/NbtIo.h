#ifndef COM_MOJANG_NBT__NbtIo_H__
#define COM_MOJANG_NBT__NbtIo_H__

//package com.mojang.nbt;

#include "CompoundTag.h"
#include "../util/DataIO.h"

class NbtIo
{
public:
    static CompoundTag* read(IDataInput* dis) {
		Tag *tag = Tag::readNamedTag(dis);
		if (tag && tag->getId() == Tag::TAG_Compound)
			return (CompoundTag*) tag;
		return NULL;
    }

    static bool write(CompoundTag* tag, IDataOutput* dos) {
		if (!tag) return false;
		Tag::writeNamedTag(tag, dos);
		return false;
    }
};

#endif /*COM_MOJANG_NBT__NbtIo_H__*/
