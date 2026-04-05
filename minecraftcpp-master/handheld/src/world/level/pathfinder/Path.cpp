#include "Path.h"

int Path::p = 0;

Path::Path()
:	nodes(NULL),
	length(0),
	index(0),
	id(++p)
{
}

Path::~Path()
{
	destroy();
}

bool Path::isEmpty() const
{
	return length == 0 || nodes == NULL;
}

void Path::copyNodes( Node** nodes, int length )
{
	destroy();

	this->length = length;
	this->nodes = new Node*[length];

	for (int i = 0; i < length; ++i)
		this->nodes[i] = new Node(*nodes[i]);
}

void Path::destroy()
{
	if (nodes) {
		for (int i = 0; i < length; ++i)
			delete nodes[i];
		delete[] nodes;

		nodes = NULL;
		index = length = 0;
	}
}

Node* Path::currentPos()
{
	return nodes[index];
}

Vec3 Path::currentPos( Entity* e ) const
{
	return getPos(e, index);
}

void Path::next()
{
	index++;
}

void Path::setSize( int size )
{
	length = size;
}

int Path::getSize() const
{
	return length;
}

bool Path::isDone() const
{
	return index >= length;
}

Node* Path::last() const
{
	if (length > 0) {
		return nodes[length - 1];
	}
	return NULL;
}

Node* Path::get( int i ) const
{
	return nodes[i];
}

int Path::getIndex() const
{
	return index;
}

void Path::setIndex( int index )
{
	this->index = index;
}

Vec3 Path::getPos( Entity* e, int index ) const
{
	float x = nodes[index]->x + (int) (e->bbWidth + 1) * 0.5f;
	float z = nodes[index]->z + (int) (e->bbWidth + 1) * 0.5f;
	float y = nodes[index]->y;
	return Vec3(x, y, z);
}

bool Path::sameAs( const Path* path ) const
{
	if (!path) return false;
	if (path->length != length) return false;
	for (int i = 0; i < length; ++i) {
		Node& node = *path->nodes[i];
		if (nodes[i]->x != node.x || nodes[i]->y != node.y || nodes[i]->z != node.z)
			return false;
	}
	return true;
}

bool Path::endsIn( const Vec3& pos ) const
{
	const Node* end = last();
	if (end == NULL) return false;
	return end->x == Mth::floor(pos.x)
		&& end->y == Mth::floor(pos.y)
		&& end->z == Mth::floor(pos.z);
}

bool Path::endsInXZ( const Vec3& pos ) const
{
	const Node* end = last();
	if (end == NULL) return false;
	return end->x == Mth::floor(pos.x)
		&& end->z == Mth::floor(pos.z);
}
