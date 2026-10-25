#ifndef OCTVM_POWERMODE_H
#define OCTVM_POWERMODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum LCDModeType {
    LCD_MODE_MIN = 0x0,
    LCD_MODE_UI = 0x1,
    LCD_MODE_STILL = 0x2,
    LCD_MODE_MOVIE = 0x3,
    LCD_MODE_MAX = 0xff,
};

enum ModeTransactionType {
    POWER_STATE_TRANSACTION_BY_ID = 0x0,       /* use the config found by the mode id */
    POWER_STATE_TRANSACTION_BY_TYPE_ID = 0x1,  /* use the config found by the mode type id */
    POWER_STATE_TRANSACTION_BY_PROPERTY = 0x2, /* use the config found by property full match */
    POWER_STATE_TRANSACTION_FLAG_MAX = 0xff,
};

enum ModeEventTriggerType {
    EVENT_TRIGGER_BY_STRING = 0x0,  /* match trigger event action by event string */
    EVENT_TRIGGER_BY_ID = 0x1,      /* match trigger event action by event id */
    EVENT_TRIGGER_FLAG_MAX = 0xff,
};

#define POWER_MODE_START_EVENT "foreground_change"

/*
 * This structure for version 1 of the power_mode ABI.
 * This structure is used between octvm and uplayer OS
 */
struct power_mode {
    uint32_t   magic;           /* magic 'M''C''D' */
    uint16_t   len;             /* length of the mode_property */
    uint16_t   hdr_size;        /* sizeof(struct power_mode) */
    uint32_t   mode_id;         /* mode id, the identify for a given power mode */
    uint32_t   type_id;         /* mode type id, use different when config different */
    uint32_t   autosave;        /* indicate whether autosave should enable */
    char       mode_property[0];/* mode property, a string like mode name, type, and etc */
} __attribute__((__packed__));

/*
 * This structure for version 1 of the trigger_event ABI.
 * This structure is used between octvm and uplayer OS
 */
struct trigger_event {
    uint32_t   magic;        /* magic 'M''C''D' */
    uint16_t   len;          /* length of the event_str */
    uint16_t   hdr_size;     /* sizeof(struct trigger_event) */
    uint32_t   event_id;     /* trigger event id */
    char       event_str[0]; /* trigger event string for matching actions */
} __attribute__((__packed__));

/*
 * event action type
 * for easy definition: parameter using 'const char *'
 */
typedef int32_t (*ActionImpl)(const char *);

#ifdef __cplusplus
}
#endif

#endif
