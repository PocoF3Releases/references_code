#define RUNIN_RESULT_FILE "/sdcard/resultTest.txt"
#define TSET_ITEM "imei:sn:runin_result:fail_loop:DDRSorting:Reboot:Vibrator:DDRTool:Video:RearCameraMain:RearCameraAux:FrontCameraMain:Sensor:Mic:Speaker:eMMC:Receiver:LCD"

enum mem_arg {
	SHOW_RESULT,
	HELP,
	VERSION
};
typedef struct{
	char imei[16];
	char sn[32];
	char runin_result[16];
	char fail_loop[16];
	char DDRSorting[16];
	char Reboot[16];
	char Vibrator[16];
	char DDRTool[16];
	char Video[16];
	char RearCameraMain[16];
	char RearCameraAux[16];
	char FrontCameraMain[16];
	char Sensor[16];
	char Mic[16];
	char Speaker[16];
	char eMMC[16];
	char Receiver[16];
	char LCD[16];
} runin_info;