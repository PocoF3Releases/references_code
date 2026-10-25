/*
 * Copyright (C) 2020 The Android Open Source Project
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

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include <log/log.h>
#include <log/log_read.h>

#include "LogStatistics.h"
#include "LogWriter.h"

// These structs are packed into a single chunk of memory for each log type within a
// SerializedLogChunk object.  Their message is contained immediately at the end of the struct.  The
// address of the next log in the buffer is *this + sizeof(SerializedLogEntry) + msg_len_.  If that
// value would overflow the chunk of memory associated with the SerializedLogChunk object, then a
// new SerializedLogChunk must be allocated to contain the next SerializedLogEntry.
class __attribute__((packed)) SerializedLogEntry {
  public:
    SerializedLogEntry(uid_t uid, pid_t pid, pid_t tid, uint64_t sequence, log_time realtime,
                       uint16_t len)
        : uid_(uid),
          pid_(pid),
          tid_(tid),
          sequence_(sequence),
          realtime_(realtime),
          msg_len_(len) {}
    SerializedLogEntry(const SerializedLogEntry& elem) = delete;
    SerializedLogEntry& operator=(const SerializedLogEntry& elem) = delete;
    ~SerializedLogEntry() {
        // Never place anything in this destructor.  This class is in place constructed and never
        // destructed.
    }

    LogStatisticsElement ToLogStatisticsElement(log_id_t log_id) const {
        return LogStatisticsElement{
                .uid = uid(),
                .pid = pid(),
                .tid = tid(),
                .tag = IsBinary(log_id) ? MsgToTag(msg(), msg_len()) : 0,
                .realtime = realtime(),
                .msg = msg(),
                .msg_len = msg_len(),
                .dropped_count = 0,
                .log_id = log_id,
                .total_len = total_len(),
                // MIUI Add
                .is_filtered = parse_is_filtered(),
                .filter_log_num = parse_filter_log_num(),
                // MIUI End
        };
    }

    bool Flush(LogWriter* writer, log_id_t log_id) const {
        struct logger_entry entry = {};

        entry.hdr_size = sizeof(struct logger_entry);
        entry.lid = log_id;
        entry.pid = pid();
        entry.tid = tid();
        entry.uid = uid();
        entry.sec = realtime().tv_sec;
        entry.nsec = realtime().tv_nsec;
        entry.len = msg_len();

        return writer->Write(entry, msg());
    }

    uid_t uid() const { return uid_; }
    pid_t pid() const { return pid_; }
    pid_t tid() const { return tid_; }
    uint16_t msg_len() const { return msg_len_; }
    uint64_t sequence() const { return sequence_; }
    log_time realtime() const { return realtime_; }

    char* msg() { return reinterpret_cast<char*>(this) + sizeof(*this); }
    const char* msg() const { return reinterpret_cast<const char*>(this) + sizeof(*this); }
    uint16_t total_len() const { return sizeof(*this) + msg_len_; }

    // MIUI Add
    bool parse_is_filtered() const {
        //08-19 17:27:27.149 28479 28503 W MISight_Filter_Begin: output too many logs: 5 logs in 0s, wait 10s to print!
        const char* misight_filter_begin_tag = "MISight_Filter_Begin";
        const char* TAG = msg() + 1;
        if (strcmp(TAG, misight_filter_begin_tag) == 0) {
            //LOG(WARNING) << "SerializedLogEntry::parse_is_filtered, MISight_Filter_Begin";
            return true;
        }
        return false;
    }

    int parse_filter_log_num() const {
        //08-19 17:27:27.144 28479 28503 W MISight_Filter_End: total filtered log count: 17
        const char* misight_filter_end_tag = "MISight_Filter_End";
        const char* TAG = msg() + 1;
        if (strcmp(TAG, misight_filter_end_tag) == 0) {
            const char* MSG = msg() + 1 + strlen(msg());
            char log_num[11] = {0}; // MAX int value is 2,147,483,647, has 10 digits, so use 11 bit is enough.
            strncpy(log_num,MSG + 26,10); //len of " total filtered log count:" is 26
            //LOG(WARNING) << "SerializedLogEntry::parse_filter_log_num, log_num:" << log_num;
            return atoi(log_num);
        }
        return -1;
    }
    // MIUI End

  private:
    const uint32_t uid_;
    const uint32_t pid_;
    const uint32_t tid_;
    const uint64_t sequence_;
    const log_time realtime_;
    const uint16_t msg_len_;
};
