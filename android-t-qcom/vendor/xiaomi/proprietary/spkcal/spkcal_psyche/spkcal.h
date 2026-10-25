#define Z_TO_OHM(z) ((z) * 5.85714 / 8192.0)




#ifndef FACTORY_BUILD
#define CALIB_FILE_BIN "/data/vendor/cit/cs35l41_cal.bin"
#define CALIB_FILE_TXT "/data/vendor/cit/cs35l41_cal.txt"
#define CALIB_FILE_BIN_RIGHT "/data/vendor/cit/cs35l41_cal_right.bin"
#define CALIB_FILE_TXT_RIGHT "/data/vendor/cit/cs35l41_cal_right.txt"
#else
#define CALIB_FILE_BIN "/mnt/vendor/persist/audio/cs35l41_cal.bin"
#define CALIB_FILE_TXT "/mnt/vendor/persist/audio/cs35l41_cal.txt"
#define CALIB_FILE_BIN_RIGHT "/mnt/vendor/persist/audio/cs35l41_cal_right.bin"
#define CALIB_FILE_TXT_RIGHT "/mnt/vendor/persist/audio/cs35l41_cal_right.txt"
#endif



//error number
#define ERR_MIXER (1 << 4)
#define ERR_CTRLS (1 << 5)
#define ERR_F0_RANGE (1 << 6)
#define ERR_Z_RANGE (1 << 7)
#define ERR_Z_STATUS (1 << 8)
#define ERR_F0_STATUS (1 << 9)
#define ERR_STORE (1 << 10)
#define ERR_MEDIA_RES (1 << 11)
#define ERR_MEDIA_NEW (1 << 12)
#define ERR_CHECK_FILES (1 << 13)

#define CMD_PARAMS_CAL_START      "cal_start"
#define CMD_PARAMS_CAL_END        "cal_end"
#define CMD_PARAMS_UNLOAD_FW      "cal_unfw"
#define CMD_PARAMS_LOAD_DIAG_FW   "cal_diag_fw"
#define CMD_PARAMS_CHK_VALUE "cal_chkval"
#define CMD_PARAMS_LOAD_DEF_FW "cal_def_fw"
#define CMD_PARAMS_GET_VALUE      "cal_value"
#define CMD_PARAMS_CHK_SCLOSED "cal_scheck"
#define CMD_PARAMS_CAL_MSG      "cal_message"
#define CMD_PARAMS_CAL_RCV_MSG      "cal_rcv"
#define CMD_PARAMS_DIAG_FO_MSG     "diag_f0"

#define WAIT_TIMEOUT 5000000


/* Payload struct for getting calibration result from DSP module */
struct cal_result {
//default left spk
    int32_t l_status;
    int32_t l_checksum;
    int32_t l_calz;
    int32_t l_ambient;
    int32_t l_f0_status;
    int32_t l_f0_value;
    int32_t l_f0_low_diff;
//right spk
    int32_t r_status;
    int32_t r_checksum;
    int32_t r_calz;
    int32_t r_ambient;
    int32_t r_f0_status;
    int32_t r_f0_value;
    int32_t r_f0_low_diff;
//error num
    int32_t e_number;
};


enum {
    MSG_RET_NONE = -1,
    MSG_VALUE_DONE = 1,
    MSG_INIT_DONE,
    MSG_UNLOAD_DONE,
    MSG_DEF_FW_DONE,
    MSG_DIAG_FW_DONE,
    MSG_STREAM_CLOSED,
    MSG_END,
};

