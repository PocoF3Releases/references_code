#ifndef __SPKCAL_CONFIG_H__
#define __SPKCAL_CONFIG_H__

enum {
    CALIB_FAILED = -1,
    CALIB_IDLE = 0,
    CALIB_RUNNING,
    CALIB_DONE,
};

#define CALIB_FILE_BIN "/mnt/vendor/persist/audio/crus_calr.bin"
#define CALIB_FILE_TXT "/mnt/vendor/persist/audio/crus_calr.txt"

#define AW_FILE_BIN "/mnt/vendor/persist/audio/aw_calr.bin"
#define AW_FILE_TXT "/mnt/vendor/persist/audio/aw_calr.txt"

#endif /*__SPKCAL_CONFIG_H__ */
