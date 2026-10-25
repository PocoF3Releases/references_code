#ifndef ANDROID_FSUTIL_H
#define ANDROID_FSUTIL_H

class FSUtil {
public:
    // return: error code.
    static int restoreFSOwnerAndContext();
};

#endif
