#ifndef UTIL__WeighedRandom_H__
#define UTIL__WeighedRandom_H__

//package util;

#include "Random.h"

class WeighedRandom
{
public:
    class WeighedRandomItem {
	public:
        int randomWeight;

		WeighedRandomItem()
		:	randomWeight(-128)
		{}

        WeighedRandomItem(int randomWeight)
		:	randomWeight(randomWeight)
		{}
        
        bool isValid() {
            return randomWeight >= 0;
        }
    };

	template <typename T>
    static int getTotalWeight(const T& items) {
        int totalWeight = 0;

        for (typename T::const_iterator it = items.begin(); it != items.end(); ++it)
            totalWeight += it->randomWeight;

        return totalWeight;
    }

    template <typename T>
    static const WeighedRandomItem* getRandomItem(Random* random, const T& items, int totalWeight) {
        int selection = random->nextInt(totalWeight);
        for (typename T::const_iterator it = items.begin(); it != items.end(); ++it) {
            selection -= it->randomWeight;
            if (selection < 0) {
                return &(*it);
            }
        }
        return NULL;
    }
    template<typename T>
    static const WeighedRandomItem* getRandomItem(Random* random, const T& items) {
        return getRandomItem(random, items, getTotalWeight(items));
    }

	template <typename T>
    static int getRandomItemIndex(Random* random, const T& items, int totalWeight) {
        int selection = random->nextInt(totalWeight);
        for (unsigned int i = 0; i < items.size(); ++i) {
			selection -= items[i].randomWeight;
            if (selection < 0) {
                return i;
            }
        }
        return -1;
    }
    template<typename T>
    static int getRandomItemIndex(Random* random, const T& items) {
        return getRandomItemIndex(random, items, getTotalWeight(items));
    }

//     static int getTotalWeight(WeighedRandomItem items) {
//         int totalWeight = 0;
//
//         for (unsigned int i = 0; i < items.size(); ++i) {
//             WeighedRandomItem& item = items[i];
//             totalWeight += item.randomWeight;
//         }
//         return totalWeight;
//     }

//     static WeighedRandomItem getRandomItem(Random* random, WeighedRandomItem[] items, int totalWeight) {
//
//         if (totalWeight <= 0) {
//             throw /*new*/ IllegalArgumentException();
//         }
//
//         int selection = random->nextInt(totalWeight);
//         for (unsigned int i = 0; i < items.size(); ++i) {
//             WeighedRandomItem& item = items[i];
//         //for (WeighedRandomItem item : items) {
//             selection -= item.randomWeight;
//             if (selection < 0) {
//                 return item;
//             }
//         }
//         return NULL;
//     }

//     static WeighedRandomItem getRandomItem(Random* random, WeighedRandomItem[] items) {
//         return getRandomItem(random, items, getTotalWeight(items));
//     }
};

#endif /*UTIL__WeighedRandom_H__*/
