#ifndef UTIL__IntHashMap_H__
#define UTIL__IntHashMap_H__

template <class V>
class IntHashMap
{
    static const int DEFAULT_INITIAL_CAPACITY = 16;
    static const int MAXIMUM_CAPACITY = 1 << 30;
    //static const float DEFAULT_LOAD_FACTOR = 0.75f;

    Entry<V>** table;
	int tableCapacity;
    int size;

    int threshold;
    const float loadFactor;

    volatile int modCount;

public:
    IntHashMap()
	:	loadFactor(0.75f)
	{
        threshold = (int) (DEFAULT_INITIAL_CAPACITY * DEFAULT_LOAD_FACTOR);
        table = new Entry[DEFAULT_INITIAL_CAPACITY];
    }

	~IntHashMap() {
		delete[] table;
	}

    /*public*/ int size() {
        return size;
    }

    /*public*/ bool isEmpty() {
        return size == 0;
    }

    /*public*/ V get(int key) {
        int hash = hash(key);
        for (Entry<V> e = table[indexFor(hash, table.length)]; e != NULL; e = e.next) {
            if (e.key == key) return e.value;
        }
        return NULL;
    }

    /*public*/ bool containsKey(int key) {
        return getEntry(key) != NULL;
    }

    const Entry<V> getEntry(int key) {
        int hash = hash(key);
        for (Entry<V> e = table[indexFor(hash, table.length)]; e != NULL; e = e.next) {
            if (e.key == key) return e;
        }
        return NULL;
    }

    /*public*/ void put(int key, V value) {
        int hash = hash(key);
        int i = indexFor(hash, table.length);
        for (Entry<V> e = table[i]; e != NULL; e = e.next) {
            if (e.key == key) {
                e.value = value;
            }
        }

        modCount++;
        addEntry(hash, key, value, i);
    }

    //@SuppressWarnings("unchecked")
    /*private*/ void resize(int newCapacity) {
        Entry<V>[] oldTable = table;
        int oldCapacity = oldTable.length;
        if (oldCapacity == MAXIMUM_CAPACITY) {
            threshold = INT_MAX;//Integer.MAX_VALUE;
            return;
        }

        Entry<V>[] newTable = /*new*/ Entry[newCapacity];
        transfer(newTable);
        table = newTable;
        threshold = (int) (newCapacity * loadFactor);
    }

    /*private*/ void transfer(Entry<V>[] newTable) {
        Entry<V>[] src = table;
        int newCapacity = newTable.length;
        for (int j = 0; j < src.length; j++) {
            Entry<V> e = src[j];
            if (e != NULL) {
                src[j] = NULL;
                do {
                    Entry<V> next = e.next;
                    int i = indexFor(e.hash, newCapacity);
                    e.next = newTable[i];
                    newTable[i] = e;
                    e = next;
                } while (e != NULL);
            }
        }
    }

    /*public*/ V remove(int key) {
        Entry<V> e = removeEntryForKey(key);
        return (e == NULL ? NULL : e.value);
    }

    const Entry<V> removeEntryForKey(int key) {
        int hash = hash(key);
        int i = indexFor(hash, table.length);
        Entry<V> prev = table[i];
        Entry<V> e = prev;

        while (e != NULL) {
            Entry<V> next = e.next;
            if (e.key == key) {
                modCount++;
                size--;
                if (prev == e) table[i] = next;
                else prev.next = next;
                return e;
            }
            prev = e;
            e = next;
        }

        return e;
    }

    /*public*/ void clear() {
        modCount++;
        Entry<V>[] tab = table;
        for (int i = 0; i < tab.length; i++)
            tab[i] = NULL;
        size = 0;
    }

    /**
     * Returns <tt>true</tt> if this map maps one or more keys to the specified
     * value.
     * 
     * @param value value whose presence in this map is to be tested
     * @return <tt>true</tt> if this map maps one or more keys to the specified
     *         value
     */
    /*public*/ bool containsValue(Object value) {
        if (value == NULL) return containsNullValue();

        Entry<V>[] tab = table;
        for (int i = 0; i < tab.length; i++)
            for (Entry<V> e = tab[i]; e != NULL; e = e.next)
                if (value.equals(e.value)) return true;
        return false;
    }

    /*private*/ bool containsNullValue() {
        Entry<V>[] tab = table;
        for (int i = 0; i < tab.length; i++)
            for (Entry<V> e = tab[i]; e != NULL; e = e.next)
                if (e.value == NULL) return true;
        return false;
    }

    /*private*/
	template <class V>
	class Entry<V> {
        const int key;
        V value;
        Entry<V>* next;
        const int hash;

        /**
         * Creates new entry.
         */
        Entry(int h, int k, V v, Entry<V>* n)
		:	value(v),
			next(n),
			key(k),
			hash(h)
		{
        }

        const int getKey() {
            return key;
        }

        const V getValue() {
            return value;
        }

        //@SuppressWarnings("unchecked")
        bool operator==(const Entry<V>& rhs) {
            return key == rhs.key && value == rhs.value;
		}

        const int hashCode() {
            return hash(key);
        }

        const std::string toString() {
			std::stringstream ss;
			ss << getKey() << "=" << getValue();
			return ss.str();
        }
    }
private:
    static int hash(int h) {
        h ^= (h >>> 20) ^ (h >>> 12);
        return h ^ (h >>> 7) ^ (h >>> 4);
    }

    static int indexFor(int h, int length) {
        return h & (length - 1);
    }

	void addEntry(int hash, int key, V value, int bucketIndex) {
        Entry<V> e = table[bucketIndex];
        table[bucketIndex] = /*new*/ Entry<V>(hash, key, value, e);
        if (size++ >= threshold) resize(2 * table.length);
    }
};

#endif /*UTIL__IntHashMap_H__*/
