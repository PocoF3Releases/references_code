typedef unsigned int uint32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint64;

#pragma pack(1)
typedef struct FATAL_ERROR {
	uint8 magic_str[16];
	uint8 fatal_error_count;
	uint32 len;
	char info[];
} fatal_error_struct;

enum fe_arg {
	FE_NULL,
	FE_GET,
	FE_GET_HISTORY,
	FE_SET,
	FE_CLEAR,
	FE_MAX,
};

#pragma pack()

