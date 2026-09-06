#ifndef NET_MINECRAFT_LOCALE__I18n_H__
#define NET_MINECRAFT_LOCALE__I18n_H__

//package net.minecraft.locale;

#include <map>
#include <string>

class AppPlatform;
class ItemInstance;

class I18n
{
	typedef std::map<std::string, std::string> Map;
public:
	static void loadLanguage(AppPlatform* platform, const std::string& languageCode);

	static bool get(const std::string& id, std::string& out);
    static std::string get(const std::string& id);

    //static std::string get(const std::string& id, Object... args) {
    //    return lang.getElement(id, args);
    //}
	static std::string getDescriptionString( const ItemInstance& item );

private:
	static void fillTranslations(AppPlatform* platform, const std::string& filename, bool overwrite);
	static Map _strings;
};

#endif /*NET_MINECRAFT_LOCALE__I18n_H__*/
