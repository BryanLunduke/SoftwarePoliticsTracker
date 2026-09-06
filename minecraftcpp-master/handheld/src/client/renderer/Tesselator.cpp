#include "Tesselator.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

Tesselator Tesselator::instance(sizeof(GLfloat) * MAX_FLOATS); // max size in bytes

const int VertexSizeBytes = sizeof(VERTEX);

Tesselator::Tesselator( int size )
:	size(size),
	vertices(0),
	u(0), v(0),
	_color(0),
	hasColor(false),
	hasTexture(false),
	hasNormal(false),
	p(0),
	count(0),
	_noColor(false),
	mode(0),
	xo(0), yo(0), zo(0),
	_normal(0),
	_sx(1), _sy(1),

	tesselating(false),
	vboId(-1),
	vboCounts(128),
	totalSize(0),
	accessMode(ACCESS_STATIC),
	maxVertices(size / sizeof(VERTEX)),
	_voidBeginEnd(false)
{
	vboIds = new GLuint[vboCounts];

	_varray = new VERTEX[maxVertices];

	char* a = (char*)&_varray[0];
	char* b = (char*)&_varray[1];
	LOGI("Vsize: %lu, %d\n", sizeof(VERTEX), (b-a));
}

Tesselator::~Tesselator()
{
	delete[] vboIds;
	delete[] _varray;
}

void Tesselator::init()
{
#ifndef STANDALONE_SERVER
	glGenBuffers2(vboCounts, vboIds);
#endif
}

void Tesselator::clear()
{
	accessMode = ACCESS_STATIC;
	vertices = 0;
	count = 0;
	p = 0;
	_voidBeginEnd = false;
}

int Tesselator::getVboCount() {
	return vboCounts;
}

RenderChunk Tesselator::end( bool useMine, int bufferId )
{
#ifndef STANDALONE_SERVER
	//if (!tesselating) throw /*new*/ IllegalStateException("Not tesselating!");
	if (!tesselating)
		LOGI("not tesselating!\n");

	if (!tesselating || _voidBeginEnd) return RenderChunk();

	tesselating = false;
	const int o_vertices = vertices;

	if (vertices > 0) {
		if (++vboId >= vboCounts)
			vboId = 0;

#ifdef USE_VBO
		// Using VBO, use default buffer id only if we don't send in any
		if (!useMine) {
			bufferId = vboIds[vboId];
		}
#else
		// Not using VBO - always use the next buffer object
		bufferId = vboIds[vboId];
#endif
		int access = GL_STATIC_DRAW;//(accessMode==ACCESS_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
		int bytes = p * sizeof(VERTEX);
		glBindBuffer2(GL_ARRAY_BUFFER, bufferId);
		glBufferData2(GL_ARRAY_BUFFER, bytes, _varray, access); // GL_STREAM_DRAW
		totalSize += bytes;

#ifndef USE_VBO
		// 0 1 2 3 4 5 6 7
		// x y z u v c
		if (hasTexture) {
			glTexCoordPointer2(2, GL_FLOAT, VertexSizeBytes, (GLvoid*) (3 * 4));
			glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
		}
		if (hasColor) {
			glColorPointer2(4, GL_UNSIGNED_BYTE, VertexSizeBytes, (GLvoid*) (5 * 4));
			glEnableClientState2(GL_COLOR_ARRAY);
		}
		if (hasNormal) {
			glNormalPointer(GL_BYTE, VertexSizeBytes, (GLvoid*) (6 * 4));
			glEnableClientState2(GL_NORMAL_ARRAY);
		}
		glVertexPointer2(3, GL_FLOAT, VertexSizeBytes, 0);
		glEnableClientState2(GL_VERTEX_ARRAY);

		if (mode == GL_QUADS) {
			glDrawArrays2(GL_TRIANGLES, 0, vertices);
		} else {
			glDrawArrays2(mode, 0, vertices);
		}
		//printf("drawing %d tris, size %d (%d,%d,%d)\n", vertices, p, hasTexture, hasColor, hasNormal);
		glDisableClientState2(GL_VERTEX_ARRAY);
		if (hasTexture) glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
		if (hasColor) glDisableClientState2(GL_COLOR_ARRAY);
		if (hasNormal) glDisableClientState2(GL_NORMAL_ARRAY);
#endif /*!USE_VBO*/
	}

	clear();
	RenderChunk out(bufferId, o_vertices);
	//map.insert( std::make_pair(bufferId, out.id) );
	return out;
#else
	return RenderChunk();
#endif

}

void Tesselator::begin( int mode )
{
	if (tesselating || _voidBeginEnd) {
		if (tesselating && !_voidBeginEnd)
			LOGI("already tesselating!\n");
		return;
	}
	//if (tesselating) {
	//    throw /*new*/ IllegalStateException("Already tesselating!");
	//}
	tesselating = true;

	clear();
	this->mode = mode;
	hasNormal = false;
	hasColor = false;
	hasTexture = false;
	_noColor = false;
}

void Tesselator::begin()
{
	begin(GL_QUADS);
}

void Tesselator::tex( float u, float v )
{
	hasTexture = true;
	this->u = u;
	this->v = v;
}

int Tesselator::getColor() {
	return _color;
}

void Tesselator::color( float r, float g, float b )
{
	color((int) (r * 255), (int) (g * 255), (int) (b * 255));
}

void Tesselator::color( float r, float g, float b, float a )
{
	color((int) (r * 255), (int) (g * 255), (int) (b * 255), (int) (a * 255));
}

void Tesselator::color( int r, int g, int b )
{
	color(r, g, b, 255);
}

void Tesselator::color( int r, int g, int b, int a )
{
	if (_noColor) return;

	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	if (a > 255) a = 255;
	if (r < 0) r = 0;
	if (g < 0) g = 0;
	if (b < 0) b = 0;
	if (a < 0) a = 0;

	hasColor = true;
	//if (ByteOrder.nativeOrder() == ByteOrder.LITTLE_ENDIAN) {
	if (true) {
		_color = (a << 24) | (b << 16) | (g << 8) | (r);
	} else {
		_color = (r << 24) | (g << 16) | (b << 8) | (a);
	}
}

void Tesselator::color( char r, char g, char b )
{
	color(r & 0xff, g & 0xff, b & 0xff);
}

void Tesselator::color( int c )
{
	int r = ((c >> 16) & 255);
	int g = ((c >> 8) & 255);
	int b = ((c) & 255);
	color(r, g, b);
}

//@note: doesn't care about endianess
void Tesselator::colorABGR( int c )
{
	if (_noColor) return;
	hasColor = true;
	_color = c;
}

void Tesselator::color( int c, int alpha )
{
	int r = ((c >> 16) & 255);
	int g = ((c >> 8) & 255);
	int b = ((c) & 255);
	color(r, g, b, alpha);
}

void Tesselator::vertexUV( float x, float y, float z, float u, float v )
{
	tex(u, v);
	vertex(x, y, z);
}

void Tesselator::scale2d(float sx, float sy) {
	_sx *= sx;
	_sy *= sy;
}

void Tesselator::resetScale() {
	_sx = _sy = 1;
}

void Tesselator::vertex( float x, float y, float z )
{
#ifndef STANDALONE_SERVER
	count++;

	if (mode == GL_QUADS && (count & 3) == 0) {
		for (int i = 0; i < 2; i++) {

			const int offs = 3 - i;
			VERTEX& src = _varray[p - offs];
			VERTEX& dst = _varray[p];

			if (hasTexture) {
				dst.u = src.u;
				dst.v = src.v;
			}
			if (hasColor) {
				dst.color = src.color;
			}
			//if (hasNormal) {
			//	dst.normal = src.normal;
			//}

			dst.x = src.x;
			dst.y = src.y;
			dst.z = src.z;

			++vertices;
			++p;
		}
	}

	VERTEX& vertex = _varray[p];

	if (hasTexture) {
		vertex.u = u;
		vertex.v = v;
	}
	if (hasColor) {
		vertex.color = _color;
	}
	//if (hasNormal) {
	//	vertex.normal = _normal;
	//}

	vertex.x = _sx * (x + xo);
	vertex.y = _sy * (y + yo);
	vertex.z = z + zo;

	++p;
	++vertices;

	if ((vertices & 3) == 0 && p >= maxVertices-1) {
		for (int i = 0; i < 3; ++i)
			printf("Overwriting the vertex buffer! This chunk/entity won't show up\n");
		clear();
	}
#endif
}

void Tesselator::noColor()
{
	_noColor = true;
}

void Tesselator::setAccessMode(int mode)
{
	accessMode = mode;
}

void Tesselator::normal( float x, float y, float z )
{
	static int _warn_t = 0;
	if ((++_warn_t & 32767) == 1)
		LOGI("WARNING: Can't use normals (Tesselator::normal)\n");
	return;

	if (!tesselating) printf("But..");
	hasNormal = true;
	char xx = (char) (x * 128);
	char yy = (char) (y * 127);
	char zz = (char) (z * 127);

	_normal = xx | (yy << 8) | (zz << 16);
}

void Tesselator::offset( float xo, float yo, float zo ) {
	this->xo = xo;
	this->yo = yo;
	this->zo = zo;
}

void Tesselator::addOffset( float x, float y, float z ) {
	xo += x;
	yo += y;
	zo += z;
}

void Tesselator::offset( const Vec3& v ) {
	xo = v.x;
	yo = v.y;
	zo = v.z;
}

void Tesselator::addOffset( const Vec3& v ) {
	xo += v.x;
	yo += v.y;
	zo += v.z;
}

void Tesselator::draw()
{
#ifndef STANDALONE_SERVER
	if (!tesselating)
		LOGI("not (draw) tesselating!\n");

	if (!tesselating || _voidBeginEnd)
		return;

	tesselating = false;

	if (vertices > 0) {
		if (++vboId >= vboCounts)
			vboId = 0;

		int bufferId = vboIds[vboId];
		
		int access = GL_DYNAMIC_DRAW;//(accessMode==ACCESS_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
		int bytes = p * sizeof(VERTEX);
		glBindBuffer2(GL_ARRAY_BUFFER, bufferId);
		glBufferData2(GL_ARRAY_BUFFER, bytes, _varray, access); // GL_STREAM_DRAW

		if (hasTexture) {
			glTexCoordPointer2(2, GL_FLOAT, VertexSizeBytes, (GLvoid*) (3 * 4));
			//glTexCoordPointer2(2, GL_FLOAT, VertexSizeBytes, (GLvoid*) &_varray->u);
			glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
		}
		if (hasColor) {
			glColorPointer2(4, GL_UNSIGNED_BYTE, VertexSizeBytes, (GLvoid*) (5 * 4));
			//glColorPointer2(4, GL_UNSIGNED_BYTE, VertexSizeBytes, (GLvoid*) &_varray->color);
			glEnableClientState2(GL_COLOR_ARRAY);
		}
		//if (hasNormal) {
		//	glNormalPointer(GL_BYTE, VertexSizeBytes, (GLvoid*) (6 * 4));
		//	glEnableClientState2(GL_NORMAL_ARRAY);
		//}
		//glVertexPointer2(3, GL_FLOAT, VertexSizeBytes, (GLvoid*)&_varray);
		glVertexPointer2(3, GL_FLOAT, VertexSizeBytes, 0);
		glEnableClientState2(GL_VERTEX_ARRAY);

		if (mode == GL_QUADS) {
			glDrawArrays2(GL_TRIANGLES, 0, vertices);
		} else {
			glDrawArrays2(mode, 0, vertices);
		}

		glDisableClientState2(GL_VERTEX_ARRAY);
		if (hasTexture) glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
		if (hasColor) glDisableClientState2(GL_COLOR_ARRAY);
		//if (hasNormal) glDisableClientState2(GL_NORMAL_ARRAY);
	}

	clear();
#endif
}

void Tesselator::voidBeginAndEndCalls(bool doVoid) {
	_voidBeginEnd = doVoid;
}

void Tesselator::enableColor() {
	_noColor = false;
}
