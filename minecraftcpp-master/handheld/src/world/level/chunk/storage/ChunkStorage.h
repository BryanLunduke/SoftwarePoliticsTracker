#ifndef NET_MINECRAFT_WORLD_LEVEL_CHUNK_STORAGE__ChunkStorage_H__
#define NET_MINECRAFT_WORLD_LEVEL_CHUNK_STORAGE__ChunkStorage_H__

//package net.minecraft.world.level.chunk.storage;

class Level;
class LevelChunk;

/**@todo: load, save* threw exceptions - I think we need to substitute
		  it for something else since it hasn't very good support on
		  all embedded devices */

/*was-interface*/
class ChunkStorage {
public:
    virtual ~ChunkStorage() {}
    virtual LevelChunk* load(Level* level, int x, int z) {
		return NULL;
	}
	virtual void save(Level* level, LevelChunk* levelChunk) {}
	virtual void saveEntities(Level* level, LevelChunk* levelChunk) {}
	virtual void saveAll(Level* level, std::vector<LevelChunk*>& levelChunks) {
		for (unsigned int i = 0; i < levelChunks.size(); ++i) save(level, levelChunks[i]);
	}
    
	virtual void tick() {}
	virtual void flush() {}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_CHUNK_STORAGE__ChunkStorage_H__*/
