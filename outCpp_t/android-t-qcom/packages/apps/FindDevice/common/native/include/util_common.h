#ifndef ANDROID_UTIL_COMMON_H
#define ANDROID_UTIL_COMMON_H


#include <vector>

template <class T, class Alloc>
inline const T * addr(const std::vector<T, Alloc> & v) {
    static const T * fake = NULL;
    if (!fake) {
        fake = new T [0];
    }
    return v.empty() ? fake : &v[0];
}

template <class T, class Alloc>
inline T * addr(std::vector<T, Alloc> & v) {
    static T * fake = NULL;
    if (!fake) {
        fake = new T [0];
    }
    return v.empty() ? fake : &v[0];
}

#endif //ANDROID_UTIL_COMMON_H
