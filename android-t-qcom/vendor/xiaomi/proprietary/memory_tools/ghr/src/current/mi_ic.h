
static const char *nand_head = "imei:ufsid:rtbb:uecc:tbw:pe_cycle:pe_cycle_slc:eol_c:eol_b\
:tbr:reserved_b:spocount:sporcount:read_reclaim:init_count:vcc_count\
:vccq_count:ffu_count:min_temperature:max_temperature:bitflip_detect_count\
:bitflip_correct_count:bEDCCount:bChecksumCount:bUndefINSTCount:mdc:clk";

typedef struct NAND_VALUE_ST {
	char imei[64];
	char ufsid[128];
	int rtbb;
	int uecc;
	int tbw;
	int pe_cycle_min_tlc;
	int pe_cycle_max_tlc;
	int pe_cycle;
	int pe_cycle_min_slc;
	int pe_cycle_max_slc;
	int pe_cycle_slc;
	int eol_c;
	int eol_b;
	int tbr;
	int reserved_b;
	int spocount;
	int sporcount;
	int read_reclaim;
	int init_count;
	int vcc_count;
	int vccq_count;
	int ffu_count;
	int exhausted_life_slc;
	int exhausted_life_tlc;
	int read_retry;
	double min_temperature;
	double max_temperature;
	double pwon_min_temperature;
	double pwon_max_temperature;
	int bitflip_detect_count;
	int bitflip_correct_count;
	int bEDCCount;
	int bChecksumCount;
	int bUndefINSTCount;
	int mdc;
	int clk;
} nand_value_struct;

