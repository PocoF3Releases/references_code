typedef unsigned int uint32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint64;
#define PROMPTER_LIGHTING_LED  1 << 0
#define PROMPTER_DISPLAY_RESULT  1 << 1
#define PROMPTER_VIB_EVERY_CYCLE  1 << 2
#define PROMPTER_CAN_STOP_BY_PD  1 << 3

#define WL_PATTERN_TEST 1 << 0
#define GSORT_MAGIC_STR "gsort"
#define TRIGGER_TEST_CS0   (1<<0)
#define TRIGGER_TEST_CS1 (1<<1)
#define TRIGGER_2WL_GSORT (1<<2)

#define CID_MANFID_SANDISK	0x2
#define CID_MANFID_TOSHIBA 	0x11
#define CID_MANFID_MICRON	0x13
#define CID_MANFID_SAMSUNG	0x15
#define CID_MANFID_HYNIX	0x90
#define CID_MANFID_NUMONYX_MICRON 0xfe

#define PLATFORM_LIST "taro kalama ukee"
#define TRIGGER_PARTITION "/dev/block/by-name/xbl_sc_test_mode"
#define DDR_PARTITION "/dev/block/by-name/ddr"
#define TRIGGER_PART_SIZE 64 * 1024
#define TRIGGER_STRUCT_SIZE 24

#ifdef MTK_FLAG
#pragma pack(4)
typedef struct DDR_ST {
	uint8 magic_str[8];
	uint8 gsort_version; //currently not used
	uint16 gsort_size;

	uint32 global_clk_in_khz;
	uint8 test_trigger_bitmap;
	uint8 test_history_bitmap;
	uint8 retest_count;// set_by_caller.
	uint8 test_result_bitmap;  //total result

	uint8 test_prompter;

	uint16 pattern_offset; // normally 1KB. A pair of pattern is 64bit*2.
	uint8 pattern_count;

	uint64 pattern_error_bitmap;//max64.
	uint16 result_offset; //normally 2KB. set_by_caller.1means pass.
	uint8 result_count;
	uint8 result_item_size;

	uint8 swap_rank;/*0:no operation 1:need swap rank; 2 swapped and tested.*/
	uint32 ddr_training_times;/*ddr training times setting*/
	uint8 ddr_training_bootflag;/*1 = ddr training untill times is 0,0 = boot to kernel*/
	uint32 ddr_train_failtimes;/*ddr training fail times*/

	uint32 checksum;//must be the last member.currently not used.
	char padding[0];
}ddr_gsort_param;
#else
#pragma pack(1)
typedef struct DDR_ST {
	uint8 magic_str[8];
	uint8 gsort_version; //currently not used
	uint16 gsort_size;

	uint32 global_clk_in_khz;
	uint8 test_trigger_bitmap;
	uint8 test_history_bitmap;
	uint8 retest_count;// set_by_caller.
	uint8 test_result_bitmap;  //total result

	uint8 test_prompter;

	uint16 pattern_offset; // normally 1KB. A pair of pattern is 64bit*2.
	uint8 pattern_count;

	uint64 pattern_error_bitmap;//max64.
	uint16 result_offset; //normally 2KB. set_by_caller.1means pass.
	uint8 result_count;
	uint8 result_item_size;

	uint32 ddr_training_times;/*ddr training times setting*/
	uint8 ddr_training_bootflag;/*1 = ddr training untill times is 0,0 = boot to kernel*/
	uint32 ddr_train_failtimes;/*ddr training fail times*/

	uint32 checksum;//must be the last member.currently not used.
	char padding[];
}ddr_gsort_param;
#endif

typedef struct DDR_ST_PATTERN {
	uint64 pattern_a;
	uint64 pattern_b;
	uint32 clk_in_khz;
	uint32 pattern_total_error;
} ddr_gsort_pattern;

typedef struct DDR_ST_RESULT {
	uint8 pattern_index;
	uint8 retest_result;//retest reset_count times,and record the pass_number.
	uint64 error_range;//
	uint64 address;
	uint64 write_data; //first error
	uint64 read_data; //first error
}ddr_gsort_result;

typedef struct RESULT_STRUCT {
	uint8 tm_cookie[8];
	uint64 version;
	uint64 status;
}result_struct;
#pragma pack()

typedef struct OUTPUT_STRUCT {
	uint64 ReadWrite;
	uint8 TestStatus;
	uint8 MemoryType;
	uint32 TotalFailCount;
	uint8 Ranks;
		uint32 RankFailCount[2];
	uint8 Channels;
		uint32 ChannelFailCount[4];
	uint32 FailDataPackets;
}__attribute__ ((packed))output_struct;
typedef struct FAILDATA_STRUCT{
	uint64 FailingAddress;
	uint64 ExpectedData;
	uint64 ReadData;
}__attribute__ ((packed))faildata_struct;
