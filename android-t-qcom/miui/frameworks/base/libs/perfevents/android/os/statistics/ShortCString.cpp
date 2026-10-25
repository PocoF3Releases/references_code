#include <string.h>
#include <cutils/compiler.h>

#include "ShortCString.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

void ShortCString::write(char* c_str) {
  value[0] = '\0';
  if (c_str != nullptr) {
    int pos = 0;
    char ch = *c_str;
    while (ch != '\0' && pos < MAX_SHORT_CSTRING_LENGTH) {
      value[pos] = ch;
      c_str++;
      pos++;
      ch = *c_str;
    }
    value[pos] = '\0';
  }
}
