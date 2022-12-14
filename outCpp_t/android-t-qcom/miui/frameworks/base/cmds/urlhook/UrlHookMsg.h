#ifndef URL_HOOK_MSG_H
#define URL_HOOK_MSG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  TYPE_DOMAIN = 1,
  TYPE_NFLOG,
  TYPE_NFQUEUE,
  TYPE_NOTIFY,
};
enum {
  KEY_HOSTNAME = 1,
  KEY_UID,
  KEY_PREFIX,
  KEY_PAYLOAD,
  KEY_MARK,
};

struct MessageStream {
  unsigned char *buf_;
  unsigned char *cur_;
  MessageStream(unsigned char *buf) : buf_(buf), cur_(buf) {}
  MessageStream(MessageStream const &ms) : buf_(ms.buf_), cur_(ms.cur_) {}

  MessageStream &put(uint8_t byte) {
    *cur_++ = byte;
    return *this;
  }
  MessageStream &put(char c) {
    *cur_++ = c;
    return *this;
  }

  template <typename T> MessageStream &put(T const &t) {
    put_little_endian(t);
    return *this;
  }

  MessageStream &put(int key, const char *buf, int len = -1) {
    if (len < 0)
      len = strlen(buf);
    return put(key, (unsigned char *)buf, len);
  }

  MessageStream &put(int key, const unsigned char *buf, int len) {
    *cur_++ = key & 0xff;
    put_little_endian((uint16_t)len);
    for (int i = 0; i < len; i++) {
      *cur_++ = buf[i];
    }
    return *this;
  }

  template <typename T> MessageStream &put(int key, T const &t) {
    *cur_++ = key & 0xff;
    put_little_endian((uint16_t)sizeof(T));
    put_little_endian(t);
    return *this;
  }

  size_t size() { return cur_ - buf_; }

private:
  template <typename T> void put_little_endian(T const &t) {
    for (size_t i = 0; i < sizeof(T); i++) {
      *cur_++ = (t >> (i * 8)) & 0xff;
    }
  }
};

void logbuff(const char *prefix, const char *buff, ssize_t count);

#endif
