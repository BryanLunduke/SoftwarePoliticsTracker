#ifndef NET_MINECRAFT_CLIENT_RENDERER__Color4_H__
#define NET_MINECRAFT_CLIENT_RENDERER__Color4_H__

class Color4
{
public:
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat a;

	Color4(GLfloat r_, GLfloat g_, GLfloat b_, GLfloat a_ = 0)
	:	r(r_),
		g(g_),
		b(b_),
		a(a_)
	{}

	Color4(int hex) {
		r = ((hex>>16)& 255) / 255.0f;
		g = ((hex>>8) & 255) / 255.0f;
		b = (hex&255) / 255.0f;
		a = (hex >> 24) / 255.0f;
	}

	void add(GLfloat val, bool modifyAlpha) {
		r += val;
		g += val;
		b += val;
		if (modifyAlpha) a += val;
		sanitize();
	}
	void mul(GLfloat val, bool modifyAlpha) {
		r *= val;
		g *= val;
		b *= val;
		if (modifyAlpha) a *= val;
		sanitize();
	}
private:
	void sanitize() {
		range(r);
		range(g);
		range(b);
		range(a);
	}
	__inline void range(GLfloat& v) {
		if (v < 0) v = 0;
		if (v > 1) v = 1;
	}
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__Color4_H__*/
