#ifndef _CHARGER_LOGO_H_
#define _CHARGER_LOGO_H_

struct batt_info {
	int BATT_WIDTH;
	int BATT_HEIGHT;
	int BATT_START_X;
	int BATT_START_Y;
	int BATT_SHELL_GAP;
	int BATT_SHELL_THICK;
} *pbatt_info, *pbatt_info_second;

struct cap_info {
	int CAP_Y;
	int CAP_WIDTH;
	int CAP_HEIGHT;
	int CAP_ENDS_HEIGHT;
} *pcap_info, *pcap_info_second;

struct digit_info {
	int DIGIT_X;
	int DIGIT_Y;
	int PER_X;
	int PER_Y;
	int PER_START_Y;
} *pdigit_info, *pdigit_info_second;
#endif
