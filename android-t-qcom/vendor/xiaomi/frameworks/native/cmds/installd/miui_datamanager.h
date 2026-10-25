/*
 * Copyright (C) 2019 XIAOMI
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
#ifndef MIUI_DATAMANAGER_H_
#define MIUI_DATAMANAGER_H_

namespace android {
namespace installd {

/* flags indicate the behavior we need for process */
//static constexpr int FLAG_CLEAR_BEFORE_MOVE          = 1;
static constexpr int FLAG_COPY_INSTEADOF_RENAME        = 1 << 1;
static constexpr int FLAG_LOCK_WHEN_MOVE               = 1 << 2;

bool move_files(const char* from, const char* to,
        int32_t uid, int32_t gid, const char* se_info, int32_t flag);

int chown_r(const char* path, int32_t uid, int32_t gid);

}  // namespace installd
}  // namespace android

#endif  // MIUI_DATAMANAGER_H_