typedef unsigned int uint32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint64;

#define CID_MANFID_SANDISK    0x45
#define CID_MANFID_TOSHIBA    0x11
#define CID_MANFID_MICRON    0x13
#define CID_MANFID_SAMSUNG    0x15
#define CID_MANFID_HYNIX    0x90
#define CID_MANFID_NUMONYX_MICRON 0xfe
#define CID_MANFID_KINGSTON    0x70
#define CID_MANFID_YMTC    0x00009b

#pragma pack(1)
typedef struct MI_RIC {
	uint8 magic_str[8];
	uint16 total_running_time;
	uint16 current_running_time;
	uint16 reboot_count;
	uint16 other_count;
	uint16 wdog_count;
	uint16 kpanic_count;
	uint16 powerup_reason_count;
	uint32 uic_pa_count;
	uint32 uic_dl_count;
	uint32 uic_dme_count;
	uint32 cmd_timeout_count;
	uint32 ufs_reset_count;
	uint32 sense_key_error_count;
	uint8 ddr_vendor[16];
	uint8 ddr_size; //GB
	uint8 storage_name[16];
} mi_ric_struct;

enum fe_arg {
	FE_INIT,
	FE_RUN,
	FE_FILE,
	FE_CLEAR,
	FE_SHOW,
	FE_HELP
};


#pragma pack()

