#ifndef COM_MOJANG_NBT__ListTag_H__
#define COM_MOJANG_NBT__ListTag_H__

//package com.mojang.nbt;

#include "Tag.h"
#include <vector>

//template <class T>
class ListTag: public Tag
{
	typedef Tag super;
	typedef Tag* T;
	typedef std::vector<T> List;

public:
    ListTag()
	:	super("")
	{
    }

    ListTag(const std::string& name)
	:	super(name)
	{
    }

    void write(IDataOutput* dos) /*throws IOException*/ {
        if (list.size() > 0) type = list.front()->getId();
        else type = TAG_Byte;

        dos->writeByte(type);
        dos->writeInt(list.size());

		for (List::iterator it = list.begin(); it != list.end(); ++it) {
			(*it)->write(dos);
		}
    }

    //@SuppressWarnings("unchecked")
    void load(IDataInput* dis) /*throws IOException*/ {
        type = dis->readByte();
        int size = dis->readInt();

        list.clear();// = List();
        for (int i = 0; i < size; i++) {
            Tag* tag = Tag::newTag(type, NullString);
            tag->load(dis);
            list.insert(list.end(), tag);
        }
    }

    char getId() const {
        return TAG_List;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << list.size() << " entries of type " << Tag::getTagName(type);
        return ss.str();
    }

    void print(const std::string& prefix_, PrintStream& out) const {
		super::print(prefix_, out);

        std::string prefix = prefix_;
		out.print(prefix); out.println("{");
        prefix += "   ";

		for (List::const_iterator it = list.begin(); it != list.end(); ++it) {
			(*it)->print(prefix, out);
		}
        out.print(prefix_); out.println("}");
    }

    void add(T tag) {
        type = tag->getId();
        list.insert(list.end(), tag);
    }

    T get(int index) const {
		if (index >= size()) {
			errorState |= TAGERR_OUT_OF_BOUNDS;
			return NULL;
		}
        return *(list.begin() + index);
    }
	float getFloat(int index) {
		T tag = get(index);
		if (!tag)
			return 0;
		if (tag->getId() != TAG_Float) {
			errorState |= TAGERR_BAD_TYPE;
			return 0;
		}
		return ((FloatTag*)tag)->data;
	}

    int size() const {
        return list.size();
    }

    //@Override
    Tag* copy() const {
        ListTag* res = new ListTag(getName());
        res->type = type;

		for (List::const_iterator it = list.begin(); it != list.end(); ++it) {
			T copy = (*it)->copy();
			res->list.insert(res->list.end(), copy);
		}
		return res;
    }

    //@SuppressWarnings("rawtypes")
    //@Override
    bool equals(const Tag& rhs) const {
        if (super::equals(rhs)) {
            ListTag& o = (ListTag&) rhs;
            if (type == o.type && list.size() == o.list.size()) {
				for (List::const_iterator it = list.begin(), jt = o.list.begin(); it != list.end(); ++it, ++jt) {
					if ((*it)->equals(**jt))
						return false;
				}
				return true;
            }
        }
        return false;
    }

	void deleteChildren() {
		for (List::iterator it = list.begin(); it != list.end(); ++it) {
			if (*it) {
				(*it)->deleteChildren();
				delete (*it);
			}
		}
	}

private:
    List list;
    char type;
};


class ListTagFloatAdder {
public:
	ListTagFloatAdder()
	:	tag(NULL)
	{}
	ListTagFloatAdder(float f)
	:	tag(NULL)
	{
		operator() (f);
	}
	ListTagFloatAdder(ListTag* tag)
	:	tag(tag)
	{}

	ListTagFloatAdder& operator() (const std::string& name, float f) {
		if (!tag) tag = new ListTag();
		tag->add( new FloatTag(name, f) );
		return *this;
	}
	ListTagFloatAdder& operator() (float f) {
		return operator() ("", f);
	}

	ListTag* tag;
};

#endif /*COM_MOJANG_NBT__ListTag_H__*/
