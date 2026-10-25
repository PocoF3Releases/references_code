#ifndef XIAOMI_TOUCH_H_
#define XIAOMI_TOUCH_H_

#include <stdint.h>

#define THP_CMD_BASE	1000
#define MAX_BUF_SIZE    256

enum water_mode_type {
    WATER_NONE      = 0,  // 模型检测结果为无水
    WATER_LITTLE    = 1,  // 模型检测结果为少水
    WATER_LOT       = 2,  // 模型检测结果为多水
    WATER_MODE_NEED_CHECK   = 3, // 设置为该类型表示要启动水检测模型
    WATER_MODE_CHECKING = 4,  //设置为该类型表示模型正在检测中，防止重复下发
    // 以下为 touchsensor 答复的值，将其转换成上面的值
    WATER_REPLY_NONE      = 300,
    WATER_REPLY_LITTLE    = 301,
    WATER_REPLY_LOTS      = 302,
    WATER_REPLY_NEED_CHECK= 303,
    WATER_REPLY_CHECKING  = 304,
};

enum MODE_CMD {
	SET_CUR_VALUE = 0,
	GET_CUR_VALUE,
	GET_DEF_VALUE,
	GET_MIN_VALUE,
	GET_MAX_VALUE,
	GET_MODE_VALUE,
	RESET_MODE,
	SET_LONG_VALUE,
};

enum MODE_TYPE {
	Touch_Game_Mode				= 0,
	Touch_Active_MODE      		= 1,
	Touch_UP_THRESHOLD			= 2,
	Touch_Tolerance				= 3,
	Touch_Aim_Sensitivity       = 4,
	Touch_Tap_Stability         = 5,
	Touch_Expert_Mode           = 6,
	Touch_Edge_Filter      		= 7,
	Touch_Panel_Orientation 	= 8,
	Touch_Report_Rate      		= 9,
	Touch_Fod_Enable       		= 10,
	Touch_Aod_Enable       		= 11,
	Touch_Resist_RF        		= 12,
	Touch_Idle_Time        		= 13,
	Touch_Doubletap_Mode   		= 14,
	Touch_Grip_Mode        		= 15,
	Touch_FodIcon_Enable   		= 16,
	Touch_Nonui_Mode       		= 17,
	Touch_Debug_Level      		= 18,
	Touch_Power_Status     		= 19,
	Touch_Mode_NUM         		= 20,
	THP_LOCK_SCAN_MODE       	= THP_CMD_BASE + 0,
	THP_FOD_DOWNUP_CTL       	= THP_CMD_BASE + 1,
	THP_SELF_CAP_SCAN         	= THP_CMD_BASE + 2,
	THP_REPORT_POINT_SWITCH 	= THP_CMD_BASE + 3,
	THP_HAL_INIT_READY     		= THP_CMD_BASE + 4,
	THP_HAL_VSYNC_MODE   		= THP_CMD_BASE + 9,
	THP_HAL_CHARGING_STATUS		= THP_CMD_BASE + 10,
	THP_HAL_REPORT_RATE       	= THP_CMD_BASE + 11,
	THP_HAL_DISPLAY_FPS       	= THP_CMD_BASE + 12,
	THP_KNOCK_FRAME_COUNT    	= THP_CMD_BASE + 13,
	THP_HAL_TOUCH_SENSOR       	= THP_CMD_BASE + 15,
};

struct abnormal_event {
	uint16_t type;
	uint16_t code;
	uint16_t value;
};

enum abnormal_event_type {
    ABNORMAL_EVENT_TYPE_ABNORMAL = 0,
};

enum abnormal_event_CODE {
    ABNORMAL_EVENT_CODE_GHOST = 0,
};


#endif // XIAOMI_TOUCH_H_
