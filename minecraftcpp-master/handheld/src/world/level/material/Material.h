#ifndef NET_MINECRAFT_WORLD_LEVEL_MATERIAL__Material_H__
#define NET_MINECRAFT_WORLD_LEVEL_MATERIAL__Material_H__

//package net.minecraft.world.level.material;

class Material
{
public:
    static const Material* air;
    static const Material* dirt;
    static const Material* wood;
    static const Material* stone;
    static const Material* metal;
    static const Material* water;
    static const Material* lava;
    static const Material* leaves;
    static const Material* plant;
	static const Material* replaceable_plant;
    static const Material* sponge;
    static const Material* cloth;
    static const Material* fire;
    static const Material* sand;
    static const Material* decoration;
    static const Material* glass;
    static const Material* explosive;
    static const Material* coral;
    static const Material* ice;
    static const Material* topSnow;
    static const Material* snow;
    static const Material* cactus;
    static const Material* clay;
    static const Material* vegetable;
    static const Material* portal;
    static const Material* cake;
	static const Material* web;

	static void initMaterials();
	static void teardownMaterials();

	virtual bool isLiquid() const {
        return false;
    }

    virtual bool letsWaterThrough() const {
        return (!isLiquid() && !isSolid());
    }

    virtual bool isSolid() const {
        return true;
    }

    virtual bool blocksLight() const {
        return true;
    }

    virtual bool isSolidBlocking() const {
        if (_neverBuildable) return false;
        return blocksMotion();
    }

	virtual bool isAlwaysDestroyable() const {
		// these materials will always drop resources when destroyed,
		// regardless of player's equipment
		return _isAlwaysDestroyable;
	}

    virtual bool blocksMotion() const {
        return true;
    }

    virtual bool isFlammable() const {
        return _flammable;
    }
	
	virtual bool isReplaceable() const {
		return _replaceable;
	}

	virtual ~Material () {}

protected:
	Material()
	:	_flammable(false),
		_neverBuildable(false),
		_isAlwaysDestroyable(true),
		_replaceable(false)
	{}

    Material* flammable() {
        _flammable = true;
        return this;
    }

    Material* neverBuildable() {
        _neverBuildable = true;
        return this;
    }

	Material* notAlwaysDestroyable() {
		_isAlwaysDestroyable = false;
		return this;
	}

	Material* replaceable() {
		_replaceable = true;
		return this;
	}

private:
	bool _flammable;
	bool _neverBuildable;
	bool _isAlwaysDestroyable;
	bool _replaceable;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_MATERIAL__Material_H__*/
