#ifndef COM_MOJANG_NBT__CompoundTag_H__
#define COM_MOJANG_NBT__CompoundTag_H__

//package com.mojang.nbt;

#include "Tag.h"
#include <map>
#include <vector>
#include <iterator>
#include <algorithm>
#include "../util/CollectionUtils.h"

#include "ByteTag.h"
#include "ShortTag.h"
#include "IntTag.h"
#include "LongTag.h"
#include "FloatTag.h"
#include "DoubleTag.h"
#include "ListTag.h"
#include "StringTag.h"
#include "ByteArrayTag.h"

class CompoundTag: public Tag
{
	typedef Tag super;
	typedef std::map<std::string, Tag*> TagMap;
public:
    CompoundTag()
	:	super("")
	{
    }

    CompoundTag(const std::string& name)
    :   super(name)
	{
    }

    void write(IDataOutput* dos) /*throws IOException*/ {
		for (TagMap::iterator it = tags.begin(); it != tags.end(); ++it) {
            Tag::writeNamedTag(it->second, dos);
        }
        dos->writeByte(Tag::TAG_End);
    }

    void load(IDataInput* dis) /*throws IOException*/ {
        tags.clear();
        Tag* tag = NULL;

        while ((tag = Tag::readNamedTag(dis)) && tag->getId() != Tag::TAG_End) {
			tags.insert(std::make_pair(tag->getName(), tag));
        }
		// <tag> is now of type TAG_End (that's new'ed). Delete it.
		delete tag;
    }

    void getAllTags(std::vector<Tag*>& tags_) const {
		//std::transform(tags.begin(), tags.end(), std::back_inserter(tags_), PairValueFunctor());
		for (TagMap::const_iterator it = tags.begin(); it != tags.end(); ++it) {
			tags_.push_back(it->second);
		}
    }

    char getId() const {
        return TAG_Compound;
    }

    void put(const std::string& name, Tag* tag) {
        tags.insert(std::make_pair(name, tag->setName(name)));
    }

    void putByte(const std::string& name, char value) {
        tags.insert(std::make_pair(name, new ByteTag(name, value)));
    }

    void putShort(const std::string& name, short value) {
        tags.insert(std::make_pair(name, new ShortTag(name, value)));
    }

    void putInt(const std::string& name, int value) {
        tags.insert(std::make_pair(name, new IntTag(name, value)));
    }

    void putLong(const std::string& name, long value) {
        tags.insert(std::make_pair(name, new LongTag(name, value)));
    }

    void putFloat(const std::string& name, float value) {
        tags.insert(std::make_pair(name, new FloatTag(name, value)));
    }

    void putDouble(const std::string& name, float value) {
        tags.insert(std::make_pair(name, new DoubleTag(name, value)));
    }

    void putString(const std::string& name, const std::string& value) {
        tags.insert(std::make_pair(name, new StringTag(name, value)));
    }

    void putByteArray(const std::string& name, TagMemoryChunk mem) {
        tags.insert(std::make_pair(name, new ByteArrayTag(name, mem)));
    }

    void putCompound(const std::string& name, CompoundTag* value) {
        tags.insert(std::make_pair(name, value->setName(name)));
    }

    void putBoolean(const std::string& string, bool val) {
        putByte(string, val ? (char) 1 : 0);
    }

    Tag* get(const std::string& name) const {
		TagMap::const_iterator it = tags.find(name);
		if (it == tags.end())
			return NULL;
		return it->second;
    }

    __inline bool contains(const std::string& name) const {
		return tags.find(name) != tags.end();
	}
	bool contains(const std::string& name, int type) const {
		Tag* tag = get(name);
		return tag && tag->getId() == type;
	}

    char getByte(const std::string& name) const {
        if (!contains(name, TAG_Byte)) return (char) 0;
        return ((ByteTag*) get(name))->data;
    }

    short getShort(const std::string& name) const {
        if (!contains(name, TAG_Short)) return (short) 0;
        return ((ShortTag*) get(name))->data;
    }

    int getInt(const std::string& name) const {
        if (!contains(name, TAG_Int)) return (int) 0;
        return ((IntTag*) get(name))->data;
    }

    long long getLong(const std::string& name) const {
        if (!contains(name, TAG_Long)) return (long long) 0;
        return ((LongTag*) get(name))->data;
    }

    float getFloat(const std::string& name) const {
        if (!contains(name, TAG_Float)) return (float) 0;
        return ((FloatTag*) get(name))->data;
    }

    double getDouble(const std::string& name) const {
        if (!contains(name, TAG_Double)) return (double) 0;
        return ((DoubleTag*) get(name))->data;
    }

    std::string getString(const std::string& name) const {
        if (!contains(name, TAG_String)) return "";
        return ((StringTag*) get(name))->data;
    }

    TagMemoryChunk getByteArray(const std::string& name) const {
        if (!contains(name, TAG_Byte_Array)) return TagMemoryChunk();
        return ((ByteArrayTag*) get(name))->data;
    }

    CompoundTag* getCompound(const std::string& name) const {
        if (!contains(name, TAG_Compound)) return new CompoundTag(name);
        return (CompoundTag*) get(name);
    }

    ListTag* getList(const std::string& name) const {
        if (!contains(name, TAG_List)) return new ListTag(name);
        return (ListTag*) get(name);
    }

    bool getBoolean(const std::string& string) const {
        return getByte(string) != 0;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << tags.size() << " entries";
        return ss.str();
    }

    void print(const std::string& prefix_, PrintStream& out) const {
        super::print(prefix_, out);
		std::string prefix = prefix_;
		out.print(prefix); out.println("{");
        prefix += "   ";
		for (TagMap::const_iterator it = tags.begin(); it != tags.end(); ++it) {
			(it->second)->print(prefix, out);
		}
        out.print(prefix_);
		out.println("}");
    }

    bool isEmpty() const {
		return tags.empty();
    }

    Tag* copy() const {
        CompoundTag* tag = new CompoundTag(getName());
        for (TagMap::const_iterator it = tags.begin(); it != tags.end(); ++it) {
			//tag->put(it->first, get(it->first)->copy());
			tag->put(it->first, it->second->copy());
        }
        return tag;
    }

    //@Override
    bool equals(const Tag& obj) const {
        if (super::equals(obj)) {
            CompoundTag& o = (CompoundTag&) obj;

			if (tags.size() != o.tags.size())
				return false;

			for (TagMap::const_iterator it = tags.begin(), jt = o.tags.begin(); it != tags.end(); ++it, ++jt) {
				if (it->first != jt->first) return false;
				if (!it->second->equals(*jt->second)) return false;
			}

			return true;
        }
        return false;
    }

	void deleteChildren() {
		TagMap::iterator it = tags.begin();
		for (; it != tags.end(); ++it) {
			if (!it->second)
				continue;
			it->second->deleteChildren();
			delete it->second;
		}
	}
private:
     TagMap tags;
};

#endif /*COM_MOJANG_NBT__CompoundTag_H__*/
