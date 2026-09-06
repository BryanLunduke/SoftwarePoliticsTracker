#include "NinePatch.h"

NinePatchDescription::NinePatchDescription( float x, float y, float x1, float x2, float x3, float y1, float y2, float y3, float w, float e, float n, float s ) :   u0(x), u1(x + x1), u2(x + x2), u3(x + x3),
	v0(y), v1(y + y1), v2(y + y2), v3(y + y3),
	w(w), e(e), n(n), s(s),
	imgW(-1),
	imgH(-1) {

}

NinePatchDescription& NinePatchDescription::transformUVForImage( const TextureData& d ) {
	return transformUVForImageSize(d.w, d.h);
}

NinePatchDescription& NinePatchDescription::transformUVForImageSize( int w, int h ) {
	if (imgW < 0)
		imgW = imgH = 1;

	const float us = (float) imgW / w; // @todo: prepare for normal blit? (e.g. mult by 256)
	const float vs = (float) imgH / h;
	u0 *= us; u1 *= us; u2 *= us; u3 *= us;
	v0 *= vs; v1 *= vs; v2 *= vs; v3 *= vs;

	imgW = w;
	imgH = h;

	return *this;
}

NinePatchDescription NinePatchDescription::createSymmetrical( int texWidth, int texHeight, const IntRectangle& src, int xCutAt, int yCutAt ) {
	NinePatchDescription patch((float)src.x, (float)src.y,// width and height of src
		(float)xCutAt, (float)(src.w-xCutAt), (float)src.w,		// u tex coordinates
		(float)yCutAt, (float)(src.h-yCutAt), (float)src.h,		// v tex coordinates
		(float)xCutAt, (float)xCutAt, (float)yCutAt, (float)yCutAt);	// border width and heights
	if (texWidth > 0) patch.transformUVForImageSize(texWidth, texHeight);
	return patch;
}

NinePatchLayer::NinePatchLayer(const NinePatchDescription& desc, const std::string& imageName, Textures* textures, float w, float h)
	:	desc(desc),
	imageName(imageName),
	textures(textures),
	w(-1), h(-1),
	excluded(0)
{
	setSize(w, h);
}

void NinePatchLayer::setSize( float w, float h ) {
	if (w == this->w && h == this->h)
		return;

	this->w = w;
	this->h = h;

	for (int i = 0; i < 9; ++i)
		buildQuad(i);
}

void NinePatchLayer::draw( Tesselator& t, float x, float y ) {
	textures->loadAndBindTexture(imageName);
	t.begin();
	t.addOffset(x, y, 0);
	for (int i = 0, b = 1; i < 9; ++i, b += b)
		if ((b & excluded) == 0)
			d(t, quads[i]);
	t.addOffset(-x, -y, 0);
	t.draw();
}

NinePatchLayer* NinePatchLayer::exclude( int excludeId ) {
	return setExcluded(excluded | (1 << excludeId));
}

NinePatchLayer* NinePatchLayer::setExcluded( int exludeBits ) {
	excluded = exludeBits;
	return this;
}

void NinePatchLayer::buildQuad( int qid ) {
	//@attn; fix
	CachedQuad& q = quads[qid];
	const int yid = qid / 3;
	const int xid = qid - 3 * yid;
	q.u0 = (&desc.u0)[xid];
	q.u1 = (&desc.u0)[xid + 1];
	q.v0 = (&desc.v0)[yid];
	q.v1 = (&desc.v0)[yid + 1];
	q.z = 0;
	getPatchInfo(xid, yid, q.x0, q.x1, q.y0, q.y1);
	/*		q.x0 = w * (q.u0 - desc.u0);
	q.y0 = h * (q.v0 - desc.v0);
	q.x1 = w * (q.u1 - desc.u0);
	q.y1 = h * (q.v1 - desc.v0);
	*/
}

void NinePatchLayer::getPatchInfo( int xc, int yc, float& x0, float& x1, float& y0, float& y1 ) {
	if		(xc == 0) { x0 = 0; x1 = desc.w; }
	else if (xc == 1) { x0 = desc.w; x1 = w - desc.e; }
	else if (xc == 2) { x0 = w-desc.e; x1 = w; }
	if      (yc == 0) { y0 = 0; y1 = desc.n; }
	else if (yc == 1) { y0 = desc.n; y1 = h - desc.s; }
	else if (yc == 2) { y0 = h-desc.s; y1 = h; }
}

void NinePatchLayer::d( Tesselator& t, const CachedQuad& q ) {
	/*
	t.vertexUV(x    , y + h, blitOffset, (float)(sx    ), (float)(sy + sh));
	t.vertexUV(x + w, y + h, blitOffset, (float)(sx + sw), (float)(sy + sh));
	t.vertexUV(x + w, y    , blitOffset, (float)(sx + sw), (float)(sy    ));
	t.vertexUV(x    , y    , blitOffset, (float)(sx    ), (float)(sy    ));
	*/

	t.vertexUV(q.x0, q.y1, q.z, q.u0, q.v1);
	t.vertexUV(q.x1, q.y1, q.z, q.u1, q.v1);
	t.vertexUV(q.x1, q.y0, q.z, q.u1, q.v0);
	t.vertexUV(q.x0, q.y0, q.z, q.u0, q.v0);
}

NinePatchFactory::NinePatchFactory( Textures* textures, const std::string& imageName ) :	textures(textures),
	imageName(imageName),
	width(1),
	height(1) {
		TextureId id = textures->loadTexture(imageName);
		if (id != Textures::InvalidId) {
			const TextureData* data = textures->getTemporaryTextureData(id);
			if (data) { // This should never be false
				width = data->w;
				height = data->h;
			}
		} else {
			LOGE("Error @ NinePatchFactory::ctor - Couldn't find texture: %s\n", imageName.c_str());
		}
}

NinePatchLayer* NinePatchFactory::createSymmetrical( const IntRectangle& src, int xCutAt, int yCutAt, float w /*= 32.0f*/, float h /*= 32.0f*/ ) {
	return new NinePatchLayer(
		NinePatchDescription::createSymmetrical(width, height, src, xCutAt, yCutAt),
		imageName, textures, w, h);
}
