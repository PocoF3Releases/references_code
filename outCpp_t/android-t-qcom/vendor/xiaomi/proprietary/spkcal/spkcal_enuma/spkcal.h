#define Z_TO_OHM(z) ((z) * 5.85714 / 8192.0)

#if defined(CONFIG_TARGET_PRODUCT_ENUMA) || defined(CONFIG_TARGET_PRODUCT_ELISH) || defined(CONFIG_TARGET_PRODUCT_DAGU)
#ifndef FACTORY_BUILD
#define CALIB_FILE_BIN "/data/vendor/cit/cs35l41_cal_spk1.bin"
#define CALIB_FILE_TXT "/data/vendor/cit/cs35l41_cal_spk1.txt"
#define CALIB_FILE_BIN_RIGHT "/data/vendor/cit/cs35l41_cal_spk2.bin"
#define CALIB_FILE_TXT_RIGHT "/data/vendor/cit/cs35l41_cal_spk2.txt"

#define CALIB_FILE_BIN2 "/data/vendor/cit/cs35l41_cal_spk3.bin"
#define CALIB_FILE_TXT2 "/data/vendor/cit/cs35l41_cal_spk3.txt"
#define CALIB_FILE_BIN_RIGHT2 "/data/vendor/cit/cs35l41_cal_spk4.bin"
#define CALIB_FILE_TXT_RIGHT2 "/data/vendor/cit/cs35l41_cal_spk4.txt"

#ifndef CONFIG_TARGET_PRODUCT_DAGU
#define CALIB_FILE_BIN3 "/data/vendor/cit/cs35l41_cal_spk5.bin"
#define CALIB_FILE_TXT3 "/data/vendor/cit/cs35l41_cal_spk5.txt"
#define CALIB_FILE_BIN_RIGHT3 "/data/vendor/cit/cs35l41_cal_spk6.bin"
#define CALIB_FILE_TXT_RIGHT3 "/data/vendor/cit/cs35l41_cal_spk6.txt"

#define CALIB_FILE_BIN4 "/data/vendor/cit/cs35l41_cal_spk7.bin"
#define CALIB_FILE_TXT4 "/data/vendor/cit/cs35l41_cal_spk7.txt"
#define CALIB_FILE_BIN_RIGHT4 "/data/vendor/cit/cs35l41_cal_spk8.bin"
#define CALIB_FILE_TXT_RIGHT4 "/data/vendor/cit/cs35l41_cal_spk8.txt"
#endif

#else
#define CALIB_FILE_BIN "/mnt/vendor/persist/audio/cs35l41_cal_spk1.bin"
#define CALIB_FILE_TXT "/mnt/vendor/persist/audio/cs35l41_cal_spk1.txt"
#define CALIB_FILE_BIN_RIGHT "/mnt/vendor/persist/audio/cs35l41_cal_spk2.bin"
#define CALIB_FILE_TXT_RIGHT "/mnt/vendor/persist/audio/cs35l41_cal_spk2.txt"

#define CALIB_FILE_BIN2 "/mnt/vendor/persist/audio/cs35l41_cal_spk3.bin"
#define CALIB_FILE_TXT2 "/mnt/vendor/persist/audio/cs35l41_cal_spk3.txt"
#define CALIB_FILE_BIN_RIGHT2 "/mnt/vendor/persist/audio/cs35l41_cal_spk4.bin"
#define CALIB_FILE_TXT_RIGHT2 "/mnt/vendor/persist/audio/cs35l41_cal_spk4.txt"

#ifndef CONFIG_TARGET_PRODUCT_DAGU
#define CALIB_FILE_BIN3 "/mnt/vendor/persist/audio/cs35l41_cal_spk5.bin"
#define CALIB_FILE_TXT3 "/mnt/vendor/persist/audio/cs35l41_cal_spk5.txt"
#define CALIB_FILE_BIN_RIGHT3 "/mnt/vendor/persist/audio/cs35l41_cal_spk6.bin"
#define CALIB_FILE_TXT_RIGHT3 "/mnt/vendor/persist/audio/cs35l41_cal_spk6.txt"

#define CALIB_FILE_BIN4 "/mnt/vendor/persist/audio/cs35l41_cal_spk7.bin"
#define CALIB_FILE_TXT4 "/mnt/vendor/persist/audio/cs35l41_cal_spk7.txt"
#define CALIB_FILE_BIN_RIGHT4 "/mnt/vendor/persist/audio/cs35l41_cal_spk8.bin"
#define CALIB_FILE_TXT_RIGHT4 "/mnt/vendor/persist/audio/cs35l41_cal_spk8.txt"
#endif

#endif

#else

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

#if defined(CONFIG_TARGET_PRODUCT_ENUMA) || defined(CONFIG_TARGET_PRODUCT_ELISH)
/* Payload struct for getting calibration result from DSP module */
struct cal_result {
// SPK1
	int32_t l_status;
	int32_t l_checksum;
	int32_t l_calz;
	int32_t l_ambient;
	int32_t l_f0_status;
	int32_t l_f0_value;
	int32_t l_f0_low_diff;
// SPK2
	int32_t r_status;
	int32_t r_checksum;
	int32_t r_calz;
	int32_t r_ambient;
	int32_t r_f0_status;
	int32_t r_f0_value;
	int32_t r_f0_low_diff;
// SPK3
	int32_t l2_status;
	int32_t l2_checksum;
	int32_t l2_calz;
	int32_t l2_ambient;
	int32_t l2_f0_status;
	int32_t l2_f0_value;
	int32_t l2_f0_low_diff;
// SPK4
	int32_t r2_status;
	int32_t r2_checksum;
	int32_t r2_calz;
	int32_t r2_ambient;
	int32_t r2_f0_status;
	int32_t r2_f0_value;
	int32_t r2_f0_low_diff;
// SPK5
	int32_t l3_status;
	int32_t l3_checksum;
	int32_t l3_calz;
	int32_t l3_ambient;
	int32_t l3_f0_status;
	int32_t l3_f0_value;
	int32_t l3_f0_low_diff;
// SPK6
	int32_t r3_status;
	int32_t r3_checksum;
	int32_t r3_calz;
	int32_t r3_ambient;
	int32_t r3_f0_status;
	int32_t r3_f0_value;
	int32_t r3_f0_low_diff;
// SPK7
	int32_t l4_status;
	int32_t l4_checksum;
	int32_t l4_calz;
	int32_t l4_ambient;
	int32_t l4_f0_status;
	int32_t l4_f0_value;
	int32_t l4_f0_low_diff;
// SPK8
	int32_t r4_status;
	int32_t r4_checksum;
	int32_t r4_calz;
	int32_t r4_ambient;
	int32_t r4_f0_status;
	int32_t r4_f0_value;
	int32_t r4_f0_low_diff;
//error num
	int32_t e_number;
};
#elif defined(CONFIG_TARGET_PRODUCT_DAGU)
/* Payload struct for getting calibration result from DSP module */
struct cal_result {
// SPK1
	int32_t l_status;
	int32_t l_checksum;
	int32_t l_calz;
	int32_t l_ambient;
	int32_t l_f0_status;
	int32_t l_f0_value;
	int32_t l_f0_low_diff;
// SPK2
	int32_t r_status;
	int32_t r_checksum;
	int32_t r_calz;
	int32_t r_ambient;
	int32_t r_f0_status;
	int32_t r_f0_value;
	int32_t r_f0_low_diff;
// SPK3
	int32_t l2_status;
	int32_t l2_checksum;
	int32_t l2_calz;
	int32_t l2_ambient;
	int32_t l2_f0_status;
	int32_t l2_f0_value;
	int32_t l2_f0_low_diff;
// SPK4
	int32_t r2_status;
	int32_t r2_checksum;
	int32_t r2_calz;
	int32_t r2_ambient;
	int32_t r2_f0_status;
	int32_t r2_f0_value;
	int32_t r2_f0_low_diff;
	//error num
	int32_t e_number;
};
#else
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
#endif

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

