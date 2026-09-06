#ifndef NET_MINECRAFT_WORLD_LEVEL_CHUNK__DataLayer_H__
#define NET_MINECRAFT_WORLD_LEVEL_CHUNK__DataLayer_H__

//package net.minecraft.world.level.chunk;

#include <cstring>

class DataLayer
{
public:
	DataLayer()
	:	data(NULL),
		length(0)
	{}

    DataLayer(int length) {
		this->length = length >> 1;
		data = new unsigned char[this->length];
		setAll(0);
		slotMax = this->length;
    }

    DataLayer(unsigned char* data, int length) {
		this->length = length >> 1;
        this->data = data;
    }

	~DataLayer() {
		delete[] data;
	}

    int get(int x, int y, int z) {
        return get(x << 11 | z << 7 | y);
    }

    void set(int x, int y, int z, int val) {
        set(x << 11 | z << 7 | y, val);
    }

	__inline int get(int pos) {
		int slot = pos >> 1;
		int part = pos & 1;

		if (part == 0) {
			return data[slot] & 0xf;
		} else {
			return (data[slot] >> 4) & 0xf;
		}
	}
	__inline void set(int pos, int val) {
        int slot = pos >> 1;
	    int part = pos & 1;

	    if (part == 0) {
		    data[slot] = ((data[slot] & 0xf0) | (val & 0xf));
	    } else {
            data[slot] = ((data[slot] & 0x0f) | ((val & 0xf) << 4));
	    }
	}

    bool isValid() {
        return data != NULL;
    }

    void setAll(int br) {
        unsigned char val = (br & (br << 4));
		memset(data, val, length);
    }

	unsigned char* data;
	int length;
	int slotMax;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_CHUNK__DataLayer_H__*/
