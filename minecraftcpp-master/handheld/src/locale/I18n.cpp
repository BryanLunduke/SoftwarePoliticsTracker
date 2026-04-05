#include "I18n.h"
#include <sstream>
#include "../AppPlatform.h"
#include "../util/StringUtils.h"
#include "../world/level/tile/Tile.h"
#include "../world/item/ItemInstance.h"
#include <ctype.h>

I18n::Map I18n::_strings;

void I18n::loadLanguage( AppPlatform* platform, const std::string& languageCode )
{
	_strings.clear();
	fillTranslations(platform, "lang/en_US.lang", true);

	if (languageCode != "en_US")
		fillTranslations(platform, "lang/" + languageCode + ".lang", true);
}

bool I18n::get( const std::string& id, std::string& out ) {
	Map::const_iterator cit = _strings.find(id);
	if (cit != _strings.end()) {
		out = cit->second;
		return true;
	}
	return false;
}

std::string I18n::get( const std::string& id )
{
	Map::const_iterator cit = _strings.find(id);
	if (cit != _strings.end())
		return cit->second;

	return id + '<';//lang.getElement(id);
}

void I18n::fillTranslations( AppPlatform* platform, const std::string& filename, bool overwrite )
{
	BinaryBlob blob = platform->readAssetFile(filename);
	if (!blob.data || blob.size <= 0)
		return;

	std::string data((const char*)blob.data, blob.size);
	std::stringstream fin(data, std::ios_base::in);

	std::string line;
	while( std::getline(fin, line) ) {
		int spos = line.find('=');
		if (spos == std::string::npos)
			continue;

		std::string key   = Util::stringTrim(line.substr(0, spos));
		Map::const_iterator cit = _strings.find(key);
		if (!overwrite && cit != _strings.end())
			continue;

		std::string value = Util::stringTrim(line.substr(spos + 1));
		_strings.insert( std::make_pair(key, value ) );
	}

	delete[] blob.data;
}

std::string I18n::getDescriptionString( const ItemInstance& item )
{
	// Convert to lower. Normally std::transform would be used, but tolower might be
	// implemented with a macro in certain C-implementations -> messing stuff up
	const std::string desc = item.getDescriptionId();

	std::string s = desc;
	std::string trans;

	// Handle special cases
	if (item.id == Tile::cloth->id)
		return get(item.getAuxValue()? "desc.wool" : "desc.woolstring");
	else if (item.id == Tile::fenceGate->id)
		return I18n::get("desc.fence");
	else if (item.id == Tile::stoneSlabHalf->id)
		return I18n::get("desc.slab");

	for (unsigned int i = 0; i < s.length(); ++i)
		s[i] = ::tolower(s[i]);

	// Replace item./tile. with desc., hopefully it's enough
	if (s[0] == 't') s = Util::stringReplace(s, "tile.", "desc.");
	if (s[0] == 'i') s = Util::stringReplace(s, "item.", "desc.");
	if (I18n::get(s, trans))
		return trans;

	// Remove all materials from the identifier, since swordWood should
	// be read as just sword
	const char* materials[] = {
		"wood",
		"iron",
		"stone",
		"diamond",
		"gold",
		"brick",
		"emerald",
		"lapis",
		"cloth"
	};

	Util::removeAll(s, materials, sizeof(materials) / sizeof(const char*));
	if (I18n::get(s, trans))
		return trans;

	std::string mapping[] = {
		"tile.workbench",	"craftingtable",
	};
	const char numMappings = sizeof(mapping) / sizeof(std::string);
	for (int i = 0; i < numMappings; i += 2) {
		if (desc == mapping[i]) {
			if (I18n::get("desc." + mapping[i+1], trans))
				return trans;
		}
	}

	return desc + " : couldn't find desc";
}
