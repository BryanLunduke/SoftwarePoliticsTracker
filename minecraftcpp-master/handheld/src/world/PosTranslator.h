#ifndef NET_MINECRAFT_WORLD_PosTranslator_H__
#define NET_MINECRAFT_WORLD_PosTranslator_H__

//package net.minecraft;

class IPosTranslator {
public:
	virtual ~IPosTranslator() {}
	virtual void to(int& x, int& y, int& z) = 0;
	virtual void to(float& x, float& y, float& z) = 0;

	virtual void from(int& x, int& y, int& z) = 0;
	virtual void from(float& x, float& y, float& z) = 0;
};

class OffsetPosTranslator: public IPosTranslator {
public:
	OffsetPosTranslator()
	:	xo(0),
		yo(0),
		zo(0)
	{}
	OffsetPosTranslator(float xo, float yo, float zo)
		:	xo(xo),
		yo(yo),
		zo(zo)
	{}
	void to  (float& x, float& y, float& z) { x += xo; y += yo; z += zo; }
	void to  (int& x, int& y, int& z)       { x += (int)xo; y += (int)yo; z += (int)zo; }
	void from(float& x, float& y, float& z) { x -= xo; y -= yo; z -= zo; }
	void from(int& x, int& y, int& z)       { x -= (int)xo; y -= (int)yo; z -= (int)zo; }

	float xo, yo, zo;
};

#endif /*NET_MINECRAFT_WORLD_PosTranslator_H__*/
