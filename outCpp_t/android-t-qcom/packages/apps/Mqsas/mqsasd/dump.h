/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _DUMP_H_
#define _DUMP_H_

#include <vector>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <utils/Vector.h>

namespace android {

    class CaptureLogUtil {
    public:
        struct DumpAction {
            std::string title;
            std::vector<std::string> cmd;
            int timeout;
        };

        struct DumpData {
            // Path of the file.
            std::string name;

            // Open file descriptor for the file.
            android::base::unique_fd fd;

            // Modification time of the file.
            time_t mtime;
        };

        void dump_all(String16& type,
                    String16& headline,
                    Vector<String16>& actions,
                    Vector<String16>& params,
                    bool offline,
                    int32_t id,
                    bool upload,
                    String16& where,
                    Vector<String16>& include_files,
                    bool isRecordInLocal);

         void dump_lite_bugreport(const char* log_dir, const char* log_name);

         void dump_for_actions(const char* zip_dir, const char* log_path,
                            std::vector<DumpAction> &actions);

         void restart_adbd(bool restart);

         bool create_file_write(const char* path, const char *buff);

         // some methods for persist
         int create_file_in_persist(const char* path);

         void write_to_persist_file(const char* path, const char *buff);

         void read_file(const char* path,String8& content);

         void get_files_from_persist(String8& files);

         void flash_debugpolicy(int32_t type);

    private:
        static int RUN_CMD_DEFAULT_TIMEOUT;
        bool waitpid_with_timeout(pid_t pid, int timeout_seconds, int* status);

        int do_compress(const char* filepath,
                        const char* zip_dir,
                        Vector<String16>* included_files,
                        int timeout=60,
                        bool isRecordInLocal = false);

        void include_zip_files(const char *input, std::vector<std::string> &zip_cmd, int* index);

        void encript_zip(const char* basename);

        /* prints all the system properties */
        void print_properties(const char* output);

        /* forks a command and waits for it to finish -- timeout default 60s*/
        int run_command(const std::vector<std::string>& full_cmds,
                        const char* output,
                        int timeout=RUN_CMD_DEFAULT_TIMEOUT);

        void redirect_output(const char* path);

        /* Gets the dmesg output for the kernel */
        void do_dmesg(const char* output);

        char* find_candidate_log_file(const char* type,
                                    bool offline,
                                    int id,
                                    const char* headline,
                                    bool upload,
                                    const char* where,
                                    bool isRecordInLocal = false);

        char* get_online_file(const char* type,
                            int id,
                            const char* headline,
                            bool upload,
                            const char* online_dir);

        char* get_offline_file(const char* type, const char* headline, const char* offline_dir);

        void copy_offlinefile(const std::string file);
    };
}

#endif /* _DUMP_H_ */
