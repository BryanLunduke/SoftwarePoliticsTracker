#ifndef COLLECTIONUTILS_H__
#define COLLECTIONUTILS_H__

struct PairKeyFunctor {
    template <typename T>
    typename T::first_type operator()(T& pair) const {
        return pair.first;
    }
};
struct PairValueFunctor {
    template <typename T>
    typename T::second_type operator()(T& pair) const {
        return pair.second;
    }
};

#endif /*COLLECTIONUTILS_H__*/
