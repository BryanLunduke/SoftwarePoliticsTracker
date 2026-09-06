#include "Tag.h"

#include "EndTag.h"
#include "ByteTag.h"
#include "ShortTag.h"
#include "IntTag.h"
#include "LongTag.h"
#include "FloatTag.h"
#include "DoubleTag.h"
#include "ByteArrayTag.h"
#include "StringTag.h"
#include "ListTag.h"
#include "CompoundTag.h"


/*static*/ const std::string Tag::NullString = "";


Tag::Tag(const std::string& name)
:	name(name),
	errorState(0)
{
}

/*virtual*/
bool Tag::equals(const Tag& rhs) const {
	return getId() == rhs.getId()
		&& name == rhs.name;
}

/*virtual*/
void Tag::print(PrintStream& out) const {
    print("", out);
}

/*virtual*/
void Tag::print(const std::string& prefix, PrintStream& out) const {
    std::string name = getName();

    out.print(prefix);
    out.print(getTagName(getId()));
    if (name.length() > 0) {
        out.print("(\"" + name + "\")");
    }
    out.print(": ");
    out.println(toString());
}

/*virtual*/
Tag* Tag::setName(const std::string& name) {
    this->name = name;
    return this;
}

/*virtual*/
std::string Tag::getName() const {
    return name;
}

/*static*/
Tag* Tag::readNamedTag(IDataInput* dis) /*throws IOException*/ {
    char type = dis->readByte();
    if (type == Tag::TAG_End) return new EndTag();

    Tag* tag = newTag(type, dis->readString());
	if (!tag)
		return NULL;

    tag->load(dis);
    return tag;
}

/*static*/
void Tag::writeNamedTag(Tag* tag, IDataOutput* dos) /*throws IOException*/ {
    dos->writeByte(tag->getId());
    if (tag->getId() == Tag::TAG_End) return;

	dos->writeString(tag->getName());
    tag->write(dos);
}

/*static*/
Tag* Tag::newTag(char type, const std::string& name)
{
    switch (type) {
    case TAG_End:
        return new EndTag();
    case TAG_Byte:
        return new ByteTag(name);
    case TAG_Short:
        return new ShortTag(name);
    case TAG_Int:
        return new IntTag(name);
    case TAG_Long:
        return new LongTag(name);
    case TAG_Float:
        return new FloatTag(name);
    case TAG_Double:
        return new DoubleTag(name);
    case TAG_Byte_Array:
        return new ByteArrayTag(name);
    case TAG_String:
        return new StringTag(name);
    case TAG_List:
        return new ListTag(name);
    case TAG_Compound:
        return new CompoundTag(name);
    }
    return NULL;
}

/*static*/
std::string Tag::getTagName(char type) {
    switch (type) {
    case TAG_End:
        return "TAG_End";
    case TAG_Byte:
        return "TAG_Byte";
    case TAG_Short:
        return "TAG_Short";
    case TAG_Int:
        return "TAG_Int";
    case TAG_Long:
        return "TAG_Long";
    case TAG_Float:
        return "TAG_Float";
    case TAG_Double:
        return "TAG_Double";
    case TAG_Byte_Array:
        return "TAG_Byte_Array";
    case TAG_String:
        return "TAG_String";
    case TAG_List:
        return "TAG_List";
    case TAG_Compound:
        return "TAG_Compound";
    }
    return "UNKNOWN";
}
