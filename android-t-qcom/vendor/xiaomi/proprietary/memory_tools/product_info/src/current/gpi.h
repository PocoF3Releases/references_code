#define INQURIY_VENDOR_ID_SIZE  8
#define INQURIY_PRODUCT_ID_SIZE  16
#define INQURIY_FW_VERSION_SIZE  4

#define MV "/proc/mv"

static const char *pi_head = "product:fsn:imei1:imei2:cpuid:ddr_vendor:ddr_size\
:nand_vendor:nand_size:ufsid:nand_pn";

typedef struct PRODUCT_INFO_ST {
	char product[16];
	char fsn[16];
	char imei1[16];
	char imei2[16];
	char cpuid[64];
	char ddr_vendor[16];
	char ddr_size[8];
	char nand_vendor[16];
	char nand_size[8];
	char ufsid[64];
	char nand_pn[16];
} product_info_struct;

void show_usage();
