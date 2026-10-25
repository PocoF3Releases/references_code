/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <ziparchive/zip_archive.h>
#include "otautil/sysutil.h"

#include "verifier.h"

enum class PackageType {
  kMemory,
  kFile,
};

// This class serves as a wrapper for an OTA update package. It aims to provide the common
// interface for both packages loaded in memory and packages read from fd.
class Package : public VerifierInterface {
 public:
  static std::unique_ptr<Package> CreateMemoryPackage(
      const std::string& path, const std::function<void(float)>& set_progress);
  static std::unique_ptr<Package> CreateMemoryPackage(
      std::vector<uint8_t> content, const std::function<void(float)>& set_progress);
  static std::unique_ptr<Package> CreateFilePackage(const std::string& path,
                                                    const std::function<void(float)>& set_progress);

  virtual ~Package() = default;

  virtual PackageType GetType() const = 0;

  virtual std::string GetPath() const = 0;

  // Opens the package as a zip file and returns the ZipArchiveHandle.
  virtual ZipArchiveHandle GetZipArchiveHandle() = 0;

  // Updates the progress in fraction during package verification.
  void SetProgress(float progress) override;

 protected:
  // An optional function to update the progress.
  std::function<void(float)> set_progress_;
};

class MemoryPackage : public Package {
public:
    // Constructs the class from a file. We will memory maps the file later.
    MemoryPackage(const std::string& path, std::unique_ptr<MemMapping> map,
                  const std::function<void(float)>& set_progress);

    // Constructs the class from the package bytes in |content|.
    MemoryPackage(std::vector<uint8_t> content, const std::function<void(float)>& set_progress);

    ~MemoryPackage() override;

    PackageType GetType() const override {
      return PackageType::kMemory;
    }

    // Memory maps the package file if necessary. Initializes the start address and size of the
    // package.
    uint64_t GetPackageSize() const override {
      return package_size_;
    }

    std::string GetPath() const override {
      return path_;
    }

    bool ReadFullyAtOffset(uint8_t* buffer, uint64_t byte_count, uint64_t offset) override;

    ZipArchiveHandle GetZipArchiveHandle() override;

    bool UpdateHashAtOffset(const std::vector<HasherUpdateCallback>& hashers, uint64_t start,
                            uint64_t length) override;
    // MIUI ADD:
    MemMapping* GetMemMapping() { return map_.get();};

private:
    const uint8_t* addr_;    // Start address of the package in memory.
    uint64_t package_size_;  // Package size in bytes.

    // The memory mapped package.
    std::unique_ptr<MemMapping> map_;
    // A copy of the package content, valid only if we create the class with the exact bytes of
    // the package.
    std::vector<uint8_t> package_content_;
    // The physical path to the package, empty if we create the class with the package content.
    std::string path_;

    // The ZipArchiveHandle of the package.
    ZipArchiveHandle zip_handle_;
};