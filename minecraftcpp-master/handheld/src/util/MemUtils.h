#ifndef MEMUTILS_H__
#define MEMUTILS_H__

template <class T>
class Ref {
public:
	//~Ref() { //@todo: add this if the pointer is created externally
	//	dec();
	//}
	void inc() { ++_count; }
	void dec() { if (--_count == 0 && _obj) delete _obj; }

	__inline short refCount() { return _count; }
	__inline bool isUnique()  { return _count == 1; }

	__inline T* obj() { return _obj; }

	T& operator->() { return *_obj; }
	void operator++() { inc(); }
	void operator--() { dec(); }

	static Ref* create(T* object) { return new Ref(object); }
private:
	Ref(T* object)
	:	_obj(object),
		_count(1) {}

	Ref(const Ref& rhs);
	Ref& operator=(const Ref& rhs);

	T* _obj;
	short _count;
};

#endif /*MEMUTILS_H__*/
