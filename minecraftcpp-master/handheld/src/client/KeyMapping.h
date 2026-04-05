#ifndef NET_MINECRAFT_CLIENT__KeyMapping_H__
#define NET_MINECRAFT_CLIENT__KeyMapping_H__

//package net.minecraft.client;
#include <string>

class KeyMapping
{
public:
    std::string name;
    int key;
    
	KeyMapping() {}

    KeyMapping(const std::string& name_, int key_)
	:	name(name_),
		key(key_)
    {}
};

#endif /*NET_MINECRAFT_CLIENT__KeyMapping_H__*/
