#ifndef ANDROID_OS_STATISTICS_SHORTCSTRING_H
#define ANDROID_OS_STATISTICS_SHORTCSTRING_H

namespace android {
namespace os {
namespace statistics {

#define MAX_SHORT_CSTRING_LENGTH 31

class ShortCString {
public:
  char value[MAX_SHORT_CSTRING_LENGTH + 1];
public:
  void write(char* c_str);
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_SHORTCSTRING_H
