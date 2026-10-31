/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight file utils in vendor aidl
 *
 */

#include <string>
#include <sys/stat.h>

namespace aidl::vendor::xiaomi::hardware::misight::FileUtil {
// create directory,
// if parent not exist, create parent and the create dir
constexpr mode_t FILE_DEFAULT_MODE = 0664;
constexpr mode_t FILE_ALL_MODE = 0777;
constexpr mode_t FILE_EXEC_MODE = 0775;


bool CreateDirectory(const std::string& path, const mode_t& mode = FILE_DEFAULT_MODE);
bool CreateDirectoryWithOwner(const std::string& path, int root, int system);
bool IsDirectory(const std::string &path);
bool FileExists(const std::string& fileName);
bool ChangeMode(const std::string& fileName, const mode_t& mode);
int32_t GetFileSize(const std::string& filePath);
bool LoadStringFromFile(const std::string& filePath, std::string& content);
bool SaveStringToFd(int fd, const std::string& content);

void DeleteFile(const std::string& filePath);
bool MoveFile(const std::string& srcFile, const std::string& dstFile);
bool SaveStringToFile(const std::string& filePath, const std::string& content, bool truncated = true);
bool IsFileLatest(const std::string& srcFile, const std::string& dstFile);
size_t GetDirectorySize(std::string& dirString);
int DeleteDirectoryOrFile(const char *path, size_t *deleted_size);

} // namespace aidl::vendor::xiaomi::hardware::misight::FileUtil

