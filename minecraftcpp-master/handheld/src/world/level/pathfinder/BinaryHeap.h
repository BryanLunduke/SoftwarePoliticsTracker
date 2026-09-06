#ifndef NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__BinaryHeap_H__
#define NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__BinaryHeap_H__

//package net.minecraft.world.level->pathfinder;

#include "Node.h"
#include <vector>

class BinaryHeap
{
public:
    BinaryHeap()
    :   _size(0),
		_maxSize(1024)
    {
		_heap = new Node*[_maxSize];
        //heap.reserve(1024);
    }

	~BinaryHeap() {
		// @todo: figure out who's managing the memory
		delete[] _heap;
	}

    Node* insert(Node* node)
    {
        if (node->heapIdx>=0) {
            LOGE("BinaryHeap::insert. Node already added!\n");
        }

		if (_size == _maxSize)
		{
			Node** newHeap = new Node*[_maxSize = _size << 1];
			for (int i = 0; i < _size; ++i)
				newHeap[i] = _heap[i];

			//System.arraycopy(heap, 0, newHeap, 0, size);
			delete[] _heap;
			_heap = newHeap;
		}

        // Insert at end and bubble up.
        _heap[_size] = node;
		//heap.push_back(node);
        node->heapIdx = _size;
        upHeap(_size++);

        return node;
    }
    
    void clear() {
        _size = 0;
    }

    Node* pop()
    {
        Node* popped = _heap[0];
        _heap[0] = _heap[--_size];
        _heap[_size] = NULL;
		//heap.pop_back(); //?
        if (_size > 0) downHeap(0);
        popped->heapIdx=-1;
        return popped;
    }

    void remove(Node* node)
    {
        // This is what node->heapIdx is for.
        _heap[node->heapIdx] = _heap[--_size];
        _heap[_size] = NULL;
		//heap.pop_back(); //?
        if (_size > node->heapIdx)
        {
            if (_heap[node->heapIdx]->f < node->f)
            {
                upHeap(node->heapIdx);
            }
            else
            {
                downHeap(node->heapIdx);
            }
        }
        // Just as a precaution: should make stuff blow up if the node is abused.
        node->heapIdx = -1;
    }

    void changeCost(Node* node, float newCost)
    {
        float oldCost = node->f;
        node->f = newCost;
        if (newCost < oldCost)
        {
            upHeap(node->heapIdx);
        }
        else
        {
            downHeap(node->heapIdx);
        }
    }

    int size()
    {
        return _size;
    }

    bool isEmpty() {
        return _size==0;
    }

private:
    void upHeap(int idx)
    {
        Node* node = _heap[idx];
        float cost = node->f;
        while (idx > 0)
        {
            int parentIdx = (idx - 1) >> 1;
            Node* parent = _heap[parentIdx];
            if (cost < parent->f)
            {
				//LOGI("idx1: %d\n", idx);
                _heap[idx] = parent;
                parent->heapIdx = idx;
                idx = parentIdx;
            }
            else break;
        }
		//LOGI("idx2: %d\n", idx);
        _heap[idx] = node;
        node->heapIdx = idx;
    }

    void downHeap(int idx)
    {
        Node* node = _heap[idx];
        float cost = node->f;

        while (true)
        {
            int leftIdx = 1 + (idx << 1);
            int rightIdx = leftIdx + 1;

            if (leftIdx >= _size) break;

            // We definitely have a left child.
            Node* leftNode = _heap[leftIdx];
            float leftCost = leftNode->f;
            // We may have a right child.
            Node* rightNode;
            float rightCost;

            if (rightIdx >= _size)
            {
                // Only need to compare with left.
                rightNode = NULL;
                rightCost = FLT_MAX;//.POSITIVE_INFINITY;
            }
            else
            {
                rightNode = _heap[rightIdx];
                rightCost = rightNode->f;
            }

            // Find the smallest of the three costs: the corresponding node
            // should be the parent.
            if (leftCost < rightCost)
            {
                if (leftCost < cost)
                {
                    _heap[idx] = leftNode;
                    leftNode->heapIdx = idx;
					//LOGI("idx3: %d\n", idx);
                    idx = leftIdx;
                }
                else break;
            }
            else
            {
                if (rightCost < cost)
                {
                    _heap[idx] = rightNode;
                    rightNode->heapIdx = idx;
					//LOGI("idx4: %d\n", idx);
                    idx = rightIdx;
                }
                else break;
            }
        }

		//LOGI("idx5: %d\n", idx);
        _heap[idx] = node;
        node->heapIdx = idx;
    }

	Node** _heap;

    int _size;
	int _maxSize;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__BinaryHeap_H__*/
