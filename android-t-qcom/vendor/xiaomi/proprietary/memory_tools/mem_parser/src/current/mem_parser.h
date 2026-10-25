#define MAP_FILE "/data/local/tmp/ss_map.log"
#define MEMTESTER_FILE "/data/local/tmp/memtester.log"
#define STORAGE_FILE "/sdcard/miTest/RuninLog.txt"
#define RUNIN_RESULT_FILE "/sdcard/resultTest.txt"
#define UEFI_LOG_FILE "/dev/logfs/UefiLog0.txt"
#define DDR_TOTAL_1 1048576
#define DDR_TOTAL_2 2097152
#define DDR_TOTAL_3 3145728
#define DDR_TOTAL_4 4194304
#define DDR_TOTAL_6 6291456
#define DDR_TOTAL_8 8388608
#define DDR_TOTAL_10 10485760
#define DDR_TOTAL_12 12582912

#define DDR_TOTAL_16 16777216
#define MAX_UIC_CNT 100

static const char *nand_head = "imei:fsn:cpuid:ufsid:vendor:density:pn\
:ghr_result:rtbb:uecc:tbw:spocount:pe_cycle:init_count:factory_bb\
:reserved_b:read_reclaim:mdt:eol_c:eol_b:phy_err_cnt:pa_err_cnt:dl_err_cnt\
:dme_err_cnt:pwr_timeout:rtbb_mbt:uecc_mbt:rtbb_before_runin:uecc_before_runin";

static const char *dram_head = "imei:cpuid:vendor:density:gsort_result\
:retraining_result:qb_result:memtester_result:ss_map_result:storage_result\
:runin_f_loop:gsort_err_pat:build_date:ddr_lot:qb_fail_clk:running_time:crash_times";

typedef struct NAND_VALUE_ST {
	char imei[32];
	char fsn[32];
	char cpuid[128];
	char ufsid[128];
	char vendor[16];
	char density[8];
	char pn[16];
	char ghr_result[8];
	char rtbb[8];
	char uecc[8];
	char tbw[8];
	char spocount[8];
	char pe_cycle[8];
	char init_count[8];
	char factory_bb[8];
	char reserved_b[8];
	char read_reclaim[8];
	char mdt[16];
	char eol_c[8];
	char eol_b[8];
	char phy_err_cnt[8];
	char pa_err_cnt[8];
	char dl_err_cnt[8];
	char dme_err_cnt[8];
	char pwr_timeout[8];
	char rtbb_mbt[8];
	char uecc_mbt[8];
	char rtbb_before_runin[8];
	char uecc_before_runin[8];
} nand_value_struct;


typedef struct DRAM_VALUE_ST {
	char imei[32];
	char cpuid[64];
	char vendor[16];
	char density[8];
	char gsort_result[8];
	char retraining_result[8];
	char qb_result[8];
	char memtester_result[8];
	char ss_map_result[8];
	char storage_result[8];
	char runin_f_loop[8];
	char gsort_err_pat[256];
	char build_date[128];
	char ddr_lot[8];
	char qb_fail_clk[16];//As ddr_fail_clk 22022/08/03
	char running_time[16];
	char crash_times[8];
} dram_value_struct;

enum mem_arg {
	NAND,
	DRAM,
	NAND_A,
	DRAM_A,
	NAND_H,
	DRAM_H,
	NAND_C,
	NAND_E,
	HELP,
	VERSION
};
