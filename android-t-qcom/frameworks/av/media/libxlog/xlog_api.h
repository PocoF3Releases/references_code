#ifndef XLOG_API
#define XLOG_API

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ARG 12

typedef enum {
    VOLUME_BOOST = 0,
    MISOUND = 1,
    VOICE_WAKEUP = 2,
    AUDIO_ERROR = 3,
    GAME_VOICE_CHANGE = 4,
    KARAOKE = 5,
    HAPTIC = 6,
    HEADPHONES = 7,
    HEAR_COMPENSATION = 8,
    HEAR_COMPENSATION_DAILYUSE = 9,
    MUSIC_PLAYBACK = 10,
    DC_DETECT = 11,
    VIDEO_TOOLS = 12,
    VIDEO_PLAY = 13,
    GAME_DEVICE = 14,
    VOICECALL_AND_VOIP = 15,
    PROJECTION_SCREEN = 16,
    RECORD_PLAYBACK_FAIL = 17,
    WAKEUP_FAIL = 18,
    SOUNDID_STATE = 19,
    AUDIOTRACK_PARAMETER = 20
} msg_id;

struct xlog_flat_msg {
     int val[MAX_ARG];
     char  *str[MAX_ARG];
};
int xlog_send(int msg_id, struct xlog_flat_msg *log);//thread safe
int xlog_send_str(int msg_id, char *arg);
int xlog_send_int(int msg_id, int *xlog_value);
int xlog_send_java(char *msg);

#ifdef __cplusplus
}
#endif

#endif