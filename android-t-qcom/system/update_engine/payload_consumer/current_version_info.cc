//
// Copyright (C) 2013 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "update_engine/payload_consumer/current_version_info.h"

#include <algorithm>
#include <utility>

#include <base/format_macros.h>
#include <base/logging.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <android-base/properties.h>

#include "update_engine/common/utils.h"
#include "update_engine/payload_consumer/payload_constants.h"
#include "update_engine/update_metadata.pb.h"

using std::string;
using std::vector;

namespace chromeos_update_engine {

namespace {

string VectorToString(const vector<std::pair<string, string>>& input,
                      const string& separator) {
  vector<string> vec;
  std::transform(input.begin(),
                 input.end(),
                 std::back_inserter(vec),
                 [](const auto& pair) {
                   return base::JoinString({pair.first, pair.second}, ": ");
                 });
  return base::JoinString(vec, separator);
}

}  // namespace

void CurrentVersionInfo::Dump() const {
  LOG(INFO) << "CurrentVersionInfo: \n" << ToString();
}

string CurrentVersionInfo::ToString() const {
  const string EMPTY = "";
  string version = android::base::GetProperty("ro.build.version.incremental", EMPTY);
  bool is_root = android::base::GetBoolProperty("ro.debuggable", false);
  string fingerprint = android::base::GetProperty("ro.build.fingerprint", EMPTY);
  string security_patch = android::base::GetProperty("ro.build.version.security_patch", EMPTY);
  string android_version = android::base::GetProperty("ro.build.version.release", EMPTY);

  vector<string> version_info;
  version_info.emplace_back(VectorToString(
      {
          {"version", version},
          {"is_root", (is_root ? "root" : "normal")},
          {"fingerprint", fingerprint},
          {"security_patch", security_patch},
          {"android_version", android_version},
      },
      "\n"));

  return base::JoinString(version_info, "\n");
}

}  // namespace chromeos_update_engine
