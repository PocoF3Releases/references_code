/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight zip pack head file
 *
 */

#include <string>
#include <vector>

#include "minizip/zip.h"
#include "zip_base.h"

namespace aidl::vendor::xiaomi::hardware::misight {

class ZipPack : public ZipBase {
public:
    ZipPack();
    ~ZipPack();

    /* Creates a ZIP file to filePath
     * and which files (specifies with AddFiles) are relative to rootDir.*/
    bool CreateZip(const std::string& filePath);

    /* add the files at paths to the ZIP file and closes this Zip file.
     * Note that the the FilePaths must be absolute path.
     * Returns true if all fiiles were added successfuly. */
    bool AddFiles(const std::vector<std::string> &paths);

    /* Closes the ZIP file.
     * Returns true if successful, false otherwise (typically if an entry failed
     * to be written).
     */
    void Close();

private:
    bool AddFileEntryToZip(const std::string& relativePath, const std::string& absolutePath);
    bool AddDirectoryEntryToZip(const std::string& path);
    bool AddFileContentToZip(const std::string &filePath);
    bool FlushFilesIfNeeded(bool force);



    // The actual zip file pointer
    zipFile zipFile_;

    // The entries that have been added but not yet written to the ZIP file.
    std::vector<std::string> pendingFiles_;

};

} // namespace aidl::vendor::xiaomi::hardware::misight

