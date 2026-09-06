#ifndef NET_MINECRAFT_CLIENT_GAMEMODE__CreatorMode_H__
#define NET_MINECRAFT_CLIENT_GAMEMODE__CreatorMode_H__

//package net.minecraft.client.gamemode;

#include "GameMode.h"
#include "../../world/PosTranslator.h"

class ICreator {
public:
	virtual ~ICreator() {}

	struct TileEvent {
		int entityId;
		int x, y, z;
		int face;

		void write(std::stringstream& ss, IPosTranslator& t) const {
			int xx = x, yy = y, zz = z;
			t.to(xx, yy, zz);
			ss << xx << "," << yy << "," << zz << "," << face << "," << entityId;
		}
	};

	template <class T>
	class EventList {
	public:
		EventList(int size) {
			_events.reserve(size);
			_maxSize = (int)size;
			clear();
		}
		void clear() {
			_index = -1;
			_size = 0;
		}
		void add(const T& item, int tick) {
			if (_size < _maxSize) {
				_events.push_back(Item());
				++_size;
			}
			Item& e = _events[_nextIndex()];
			e.item = item;
			e.timestamp = tick;
		}
		int size() const {
			return _size;
		}

		const T& operator[](int i) const {
			return _events[_getIndex(i)].item;
		}

		T& operator[](int i) {
			return _events[_getIndex(i)].item;
		}

		void write(std::stringstream& ss, IPosTranslator& t, int minTimetamp) const {
			int i = _getFirstNewerIndex(minTimetamp);
			if (i < 0)
				return;

			while (1) {
				_events[i].item.write(ss, t);
				if (i == _index) return;
				ss << "|";
				if (++i == _size) i = 0;
			}
		}

	private:
		int _getIndex(int i) const { return (1 + _index + i) % _size; }
		int _nextIndex() {
			if (++_index == _size) _index = 0;
			return _index;
		}

		int _getFirstNewerIndex(int timestamp) const {
			for (int i = _index + 1, j = 0; j < _size; ++i, ++j) {
				if (i == _size) i = 0;
				if (_events[i].timestamp >= timestamp) return i;
			}
			return -1;
		}
		struct Item {
			int timestamp;
			T item;
		};

		int _index;
		int _size;
		int _maxSize;
		std::vector<Item> _events;
	};

	virtual EventList<TileEvent>& getTileEvents() = 0;
};

class Creator;

class CreatorMode: public GameMode
{
	typedef GameMode super;
public:
    CreatorMode(Minecraft* minecraft);
	~CreatorMode();

    void startDestroyBlock(int x, int y, int z, int face);
    void continueDestroyBlock(int x, int y, int z, int face);
    void stopDestroyBlock();

	bool useItemOn(Player* player, Level* level, ItemInstance* item, int x, int y, int z, int face, const Vec3& hit);

	void tick();
	ICreator* getCreator();

	bool isCreativeType();

	void initAbilities(Abilities& abilities);

	void releaseUsingItem(Player* player);
private:
	void CreatorDestroyBlock(int x, int y, int z, int face);

	Creator* _creator;
};

#endif /*NET_MINECRAFT_CLIENT_GAMEMODE__CreatorMode_H__*/
