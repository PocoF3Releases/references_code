#ifndef FACTORYAPP_H
#define FACTORYAPP_H

#define CONFIG_ALGO_STEREO

#define SILENCE_TRACK								"/vendor/etc/spk_cal_silence.wav"
#define PINK_NOISE_TRACK							"/vendor/etc/spk_cal_pinknoise.wav"
#define TAS25XX_ALGO_CONFIG_FILE					"/vendor/etc/calib.config"
#define MAX_STRING									(300)
	
#define MAX_NUM_SPEAKERS                   			(2)
#ifdef CONFIG_ALGO_STEREO				
#define NUM_SPEAKERS                       			(2)
#else				
#define NUM_SPEAKERS                       			(1)
#endif //CONFIG_ALGO_STEREO

//TODO
//update this file accordingly
#define TAS25XX_ALGO_CALIB_BIN_FILE                     "/data/vendor/cit/tas25xx_calib.bin"
#define TAS25XX_ALGO_CALIB_TEST_FILE                    "/data/vendor/cit/tas2559_cal.txt"
#define TAS25XX_ALGO_CALIB_BIN_FILE_PERSIST             "/mnt/vendor/persist/audio/tas25xx_calib.bin"
#define TAS25XX_ALGO_CALIB_TEST_FILE_PERSIST            "/mnt/vendor/persist/audio/tas2559_cal.txt"

/*
Profiles:
0. NONE
1. MUSIC
2. VOICE
3. VOIP
4. RING
5. CALIB
 */
#define TAS25XX_ALGO_PROFILE_CALIB_F0           "CALIB"
#define TAS25XX_ALGO_PROFILE_CALIB_RE           "CALIB"
#define TAS25XX_ALGO_DISABLE                    	(0)
#define TAS25XX_ALGO_ENABLE                     	(1)
	
#define TAS25XX_ALGO_CALIB_START                	(1) /* calib init */
#define TAS25XX_ALGO_CALIB_STOP                 	(2) /* calib deinit */
#define TAS25XX_ALGO_TEST_START                 	(3) /* f0 test start */
#define TAS25XX_ALGO_TEST_STOP                  	(4) /* f0 test deinit */

//FIXME	
#define TAS25XX_ALGO_MIXER_RX_MI2S              	"QUAT_MI2S_RX Audio Mixer MultiMedia1"
#define TAS25XX_ALGO_MIXER_TX_MI2S              	"MultiMedia1 Mixer QUAT_MI2S_TX"

//FIXME
//check if this is really required for the project
//#define TAS25XX_TAS_HW_CONTROL_L                	"TAS256X Left Speaker Switch"
//#define TAS25XX_TAS_HW_CONTROL_R                	"TAS256X Right Speaker Switch"
#define TAS25XX_TAS_HW_CONTROL_R                	"TAS256X ASI Right Switch"
#define TAS25XX_TAS_HW_CONTROL_L                	"TAS256X ASI Left Switch"

#define TAS25XX_ALGO_FB_PATH_INFO               	"CAPI_V2_TAS_FEEDBACK_INFO"
#define TAS25XX_ALGO_MIXER_SET_PROFILE          	"TAS25XX_ALGO_PROFILE"
#define TAS25XX_ALGO_MIXER_SET_ENABLE           	"TAS25XX_SMARTPA_ENABLE"

#define TAS25XX_ALGO_MIXER_GET_RDC_LEFT         	"TAS25XX_GET_RE_LEFT"
#define TAS25XX_ALGO_MIXER_GET_F0_LEFT          	"TAS25XX_GET_F0_LEFT"
#define TAS25XX_ALGO_MIXER_GET_Q_LEFT           	"TAS25XX_GET_Q_LEFT"
#define TAS25XX_ALGO_MIXER_GET_TV_LEFT           	"TAS25XX_GET_TV_LEFT"

#define TAS25XX_ALGO_MIXER_GET_RDC_RIGHT        	"TAS25XX_GET_RE_RIGHT"
#define TAS25XX_ALGO_MIXER_GET_F0_RIGHT         	"TAS25XX_GET_F0_RIGHT"
#define TAS25XX_ALGO_MIXER_GET_Q_RIGHT          	"TAS25XX_GET_Q_RIGHT"
#define TAS25XX_ALGO_MIXER_GET_TV_RIGHT          	"TAS25XX_GET_TV_RIGHT"


#define TAS25XX_ALGO_MIXER_CALIB_TEST           	"TAS25XX_ALGO_CALIB_TEST"

#define TAS25XX_ALGO_INIT               			20
#define TAS25XX_ALGO_DEINIT             			21
#define TAS25XX_ALGO_RDC_L              			24
#define TAS25XX_ALGO_F0_L               			25
#define TAS25XX_ALGO_Q_L                			26
#define TAS25XX_ALGO_TV_L               			27
			
#define TAS25XX_ALGO_RDC_R              			28
#define TAS25XX_ALGO_F0_R              				29
#define TAS25XX_ALGO_Q_R               				30
#define TAS25XX_ALGO_TV_R              				31

#define TAS25XX_TO_FIX(a, q)            			((int)((a) * ((unsigned int)1<<(q))))
#define TAS25XX_TO_DBL(a, q)            			(double)((double)((double)(a)/(double)((unsigned int)1<<(q))))
#define TAS25XX_Q_FORMAT                  			(19)
#define TAS25XX_ALGO_RDC_MIN_LEFT         			(5.5)
#define TAS25XX_ALGO_RDC_MAX_LEFT         			(8.0)
#define TAS25XX_ALGO_RDC_MIN_RIGHT        			(5.5)
#define TAS25XX_ALGO_RDC_MAX_RIGHT        			(8.0)
#define TAS25XX_RE_TEST_MIN                 		(4.0)
#define TAS25XX_RE_TEST_MAX                 		(10.0)
#define TAS25XX_ALGO_F0_MIN                 		(600.0)
#define TAS25XX_ALGO_F0_MAX                 		(1000.0)
#define TAS25XX_ALGO_Q_MIN                  		(0.6)
#define TAS25XX_ALGO_Q_MAX                			(3.0)
#define CARD                                		0
#define IN_DEVICE                       			43			//TODO: Add hostless device in OEM
#define OUT_DEVICE                      			0
#define CALIB_TIME                      			2
#define TEST_TIME                       			6
#define ALL_PASS                        			0x00000000
#define RE_LOW                          			0x00000001
#define RE_HIGH                         			0x00000010
#define F0_LOW                          			0x00000100
#define F0_HIGH                         			0x00001000
#define Q_LOW                           			0x00010000
#define Q_HIGH                          			0x00100000
		
#define DISABLE_SET_RE                  			1

#endif /*FACTORYAPP_H*/
