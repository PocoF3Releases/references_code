#define INQURIY_VENDOR_ID_SIZE  8
#define INQURIY_PRODUCT_ID_SIZE  16
#define INQURIY_FW_VERSION_SIZE  4

#define FFU_PATH "/data/vendor/ffu"

#pragma pack(1)
typedef struct DEV_ST {
	uint8_t ffu_flag[INQURIY_FW_VERSION_SIZE + 1]; //0 or ffu
	uint8_t ffu_pn[INQURIY_PRODUCT_ID_SIZE + 1]; //pn
	uint8_t ffu_current_fw[INQURIY_FW_VERSION_SIZE + 1]; //current fw version
	uint8_t ffu_target_fw[INQURIY_FW_VERSION_SIZE + 1]; //target ffu version
	int32_t ffu_count; //ffu count, also is fw count
} device_struct;
#pragma pack()

#pragma pack(1)
typedef struct FW_ST {
	uint8_t vendor[INQURIY_VENDOR_ID_SIZE + 1]; //vendor
	uint8_t pn[INQURIY_PRODUCT_ID_SIZE + 1]; //pn
	uint8_t ffu_from_fw[INQURIY_FW_VERSION_SIZE + 1]; //from fw version
	uint8_t ffu_to_fw[INQURIY_FW_VERSION_SIZE + 1]; //to fw version
	uint64_t fw_start_seek; //fw start address
	int32_t fw_size; //fw size
	uint32_t fw_crc; //crc
} fw_struct;
#pragma pack()

char * shellcmd(char *cmd, char *buff, int size);
char * get_project();
char * get_vendor();
char * get_density();
char * get_pn();
char * get_mdt();
char * get_lifetimeA();
char * get_lifetimeB();
char * get_current_fw();
void do_ffu();
