/*
 * Copyright (C) 2007-2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pmsg_writer.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

#include <log/log_properties.h>
#include <log/log_read.h>
#include <private/android_logger.h>

#include "logger.h"
#include "uio.h"
//MIUI ADD:
#include <android/log.h>
//END


static atomic_int pmsg_fd;

static void GetPmsgFd() {
  // Note if open() fails and returns -1, that value is stored into pmsg_fd as an indication that
  // pmsg is not available and open() should not be retried.
  if (pmsg_fd != 0) {
    return;
  }

  int new_fd = TEMP_FAILURE_RETRY(open("/dev/pmsg0", O_WRONLY | O_CLOEXEC));

  // Unlikely that new_fd is 0, but that is synonymous with our uninitialized value, and we'd prefer
  // STDIN_FILENO != stdin, so we call open() to get a new fd value in this case.
  if (new_fd == 0) {
    new_fd = TEMP_FAILURE_RETRY(open("/dev/pmsg0", O_WRONLY | O_CLOEXEC));
    close(0);
  }

  // pmsg_fd should only be opened once.  If we see that pmsg_fd is uninitialized, we open
  // "/dev/pmsg0" then attempt to compare/exchange it into pmsg_fd.  If the compare/exchange was
  // successful, then that will be the fd used for the duration of the program, otherwise a
  // different thread has already opened and written the fd to the atomic, so close the new fd and
  // return.
  int uninitialized_value = 0;
  if (!pmsg_fd.compare_exchange_strong(uninitialized_value, new_fd)) {
    if (new_fd != -1) {
      close(new_fd);
    }
  }
}

void PmsgClose() {
  if (pmsg_fd > 0) {
    close(pmsg_fd);
  }
  pmsg_fd = 0;
}

static inline bool is_leap_year(time_t year) {
  return (! (year % 4) && (year % 100)) || !(year % 400);
}

static void nolocks_localtime(struct tm *tmp,time_t tv_sec,struct timezone *tz) {
  const time_t secs_min = 60;
  const time_t secs_hour = 3600;
  const time_t secs_day = 3600 * 24;
  time_t days;
  time_t seconds;

  tv_sec -= tz->tz_minuteswest * 60;    /*adjust for timezone*/
  tv_sec += 3600 * tz->tz_dsttime;      /*adjust for daylight time*/
  days = tv_sec / secs_day;             /*days passed since epoch*/
  seconds = tv_sec % secs_day;          /*remaining seconds*/

  tmp->tm_isdst = tz->tz_dsttime;
  tmp->tm_hour = seconds / secs_hour;
  tmp->tm_min = (seconds % secs_hour) / secs_min;
  tmp->tm_sec = (seconds % secs_hour) % secs_min;

  /* 1/1/1970 was a thursday, that is , day 4 from the pov of the tm structure where suday
  * = 0, so to calculate the day of the week we have to add 4 and take the modulo by 7.*/
  tmp->tm_wday = (days + 4) % 7;

  /*calculate the current year*/
  tmp->tm_year = 1970;
  while(1) {
    /*leap year have one day more.*/
    time_t days_this_year = 365 + is_leap_year(tmp->tm_year);
    if(days_this_year > days)
      break;
    days -= days_this_year;
    tmp->tm_year++;
  }
  tmp->tm_yday = days; /*number of day of the current year*/

  int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  mdays[1] += is_leap_year(tmp->tm_year);

  tmp->tm_mon = 0;
  while(days >= mdays[tmp->tm_mon]) {
    days -= mdays[tmp->tm_mon];
    tmp->tm_mon++;
  }

  tmp->tm_mday = days+1; /*add 1 since our 'days' is zero-based.*/
  tmp->tm_year -= 1900;
}

int PmsgWrite(log_id_t logId, struct timespec* ts, struct iovec* vec, size_t nr) {
  //MIUI MOD:
  static const unsigned headerLength = 1;
  //END
  struct iovec newVec[nr + headerLength];
  android_log_header_t header;
  android_pmsg_log_header_t pmsgHeader;
  //MIUI ADD:
  char msg[256];
  char time_buf[32];
  time_t tv_sec = 0;
  struct tm ptm;
  size_t len = 0;
  struct timezone tz;
  struct timeval tv;
  //END
  //size_t i, payloadSize;
  ssize_t ret;

  if (!__android_log_is_debuggable()) {
    if (logId != LOG_ID_EVENTS && logId != LOG_ID_SECURITY) {
      return -1;
    }

    if (logId == LOG_ID_EVENTS) {
      if (vec[0].iov_len < 4) {
        return -EINVAL;
      }

      if (SNET_EVENT_LOG_TAG != *static_cast<uint32_t*>(vec[0].iov_base)) {
        return -EPERM;
      }
    }
  }

  GetPmsgFd();

  if (pmsg_fd <= 0) {
    return -EBADF;
  }

  /*
   *  struct {
   *      // what we provide to pstore
   *      android_pmsg_log_header_t pmsgHeader;
   *      // what we provide to file
   *      android_log_header_t header;
   *      // caller provides
   *      union {
   *          struct {
   *              char     prio;
   *              char     payload[];
   *          } string;
   *          struct {
   *              uint32_t tag
   *              char     payload[];
   *          } binary;
   *      };
   *  };
   */

  pmsgHeader.magic = LOGGER_MAGIC;
  pmsgHeader.len = sizeof(pmsgHeader) + sizeof(header);
  pmsgHeader.uid = getuid();
  pmsgHeader.pid = getpid();

  header.id = logId;
  header.tid = gettid();
  header.realtime.tv_sec = ts->tv_sec;
  header.realtime.tv_nsec = ts->tv_nsec;

  //MIUI MOD:
  gettimeofday(&tv, &tz);
  tv_sec = tv.tv_sec;
  nolocks_localtime(&ptm, tv_sec, &tz);
  snprintf(time_buf, sizeof(time_buf), "%02d-%02d %02d:%02d:%02d", ptm.tm_mon + 1, ptm.tm_mday, ptm.tm_hour, ptm.tm_min, ptm.tm_sec);

#define LOGTAG_MAX 64
  char logtag[LOGTAG_MAX];
  memset(logtag, 0, LOGTAG_MAX);
  memcpy(logtag, vec[1].iov_base, std::min(vec[1].iov_len, (size_t)(LOGTAG_MAX-1))); // remian the last byte for '\0'

  snprintf(msg, (sizeof(msg) - 1),
    "%s.%03d  %d  %d %s: ", time_buf,
    header.realtime.tv_nsec / 1000000,
    pmsgHeader.pid, header.tid,
    (unsigned char *)logtag
  );

  len = strlen(msg);
  msg[len] = ' ';

  newVec[0].iov_base = (unsigned char*)msg;
  newVec[0].iov_len = len;

  newVec[1].iov_base = vec[nr - 1].iov_base;
  newVec[1].iov_len = vec[nr - 1].iov_len;

  ret = TEMP_FAILURE_RETRY(writev(pmsg_fd, newVec, (nr > 2 ? 2 : 1)));
  if (ret < 0) {
    ret = errno ? -errno : -ENOTCONN;
  }

  return ret;
}

/*
 * Virtual pmsg filesystem
 *
 * Payload will comprise the string "<basedir>:<basefile>\0<content>" to a
 * maximum of LOGGER_ENTRY_MAX_PAYLOAD, but scaled to the last newline in the
 * file.
 *
 * Will hijack the header.realtime.tv_nsec field for a sequence number in usec.
 */

static inline const char* strnrchr(const char* buf, size_t len, char c) {
  const char* cp = buf + len;
  while ((--cp > buf) && (*cp != c))
    ;
  if (cp <= buf) {
    return buf + len;
  }
  return cp;
}

/* Write a buffer as filename references (tag = <basedir>:<basename>) */
ssize_t __android_log_pmsg_file_write(log_id_t logId, char prio, const char* filename,
                                      const char* buf, size_t len) {
  size_t length, packet_len;
  const char* tag;
  char *cp, *slash;
  struct timespec ts;
  struct iovec vec[3];

  /* Make sure the logId value is not a bad idea */
  if ((logId == LOG_ID_KERNEL) ||   /* Verbotten */
      (logId == LOG_ID_EVENTS) ||   /* Do not support binary content */
      (logId == LOG_ID_SECURITY) || /* Bad idea to allow */
      ((unsigned)logId >= 32)) {    /* fit within logMask on arch32 */
    return -EINVAL;
  }

  clock_gettime(CLOCK_REALTIME, &ts);

  cp = strdup(filename);
  if (!cp) {
    return -ENOMEM;
  }

  tag = cp;
  slash = strrchr(cp, '/');
  if (slash) {
    *slash = ':';
    slash = strrchr(cp, '/');
    if (slash) {
      tag = slash + 1;
    }
  }

  length = strlen(tag) + 1;
  packet_len = LOGGER_ENTRY_MAX_PAYLOAD - sizeof(char) - length;

  vec[0].iov_base = &prio;
  vec[0].iov_len = sizeof(char);
  vec[1].iov_base = (unsigned char*)tag;
  vec[1].iov_len = length;

  for (ts.tv_nsec = 0, length = len; length; ts.tv_nsec += ANDROID_LOG_PMSG_FILE_SEQUENCE) {
    ssize_t ret;
    size_t transfer;

    if ((ts.tv_nsec / ANDROID_LOG_PMSG_FILE_SEQUENCE) >= ANDROID_LOG_PMSG_FILE_MAX_SEQUENCE) {
      len -= length;
      break;
    }

    transfer = length;
    if (transfer > packet_len) {
      transfer = strnrchr(buf, packet_len - 1, '\n') - buf;
      if ((transfer < length) && (buf[transfer] == '\n')) {
        ++transfer;
      }
    }

    vec[2].iov_base = (unsigned char*)buf;
    vec[2].iov_len = transfer;

    ret = PmsgWrite(logId, &ts, vec, sizeof(vec) / sizeof(vec[0]));

    if (ret <= 0) {
      free(cp);
      return ret ? ret : (len - length);
    }
    length -= transfer;
    buf += transfer;
  }
  free(cp);
  return len;
}
