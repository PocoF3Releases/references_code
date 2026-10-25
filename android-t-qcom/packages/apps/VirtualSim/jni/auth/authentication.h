#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <jni.h>
#include <string.h>
#include "milenage.h"
#include "comp128.h"


void authenticationUsim(uint8 *auth_apdu);
void authenticationUsimForTest(uint8 *auth_apdu);
void authenticationSim(uint8 *auth_apdu);
void authenticationSimForTest(uint8 *auth_apdu);
void authenticationFailed();
int isAuthByTz();
void authUsimBySkb(uint8 *auth_apdu);
void authUsimByTz(uint8 *auth_apdu);
void printAuthData(uint8 *auth_apdu);
int getSkbPath(uint8 **skb_path);
void authenticationCsimForTest(uint8 *auth_apdu);
void authenticationCsim(uint8 *auth_apdu);
void authCsimBySkb(uint8 *auth_apdu);
void authCsimByTz(uint8 *auth_apdu);

#define USIM_DATA_LEN 55
#define SIM_DATA_LEN 14
#define CSIM_DATA_LEN 18
#define IMSI_LEN 9
#define ICCID_LEN 10
#define OPC_LEN 16
#define KI_LEN 16
#define PLMNWACT_LEN 40
#define FPLMN_LEN 24
#define HRPDSS_LEN 16
#define R15_LEN 5

extern uint8 usim_data[USIM_DATA_LEN];
extern uint8 sim_data[SIM_DATA_LEN];
extern uint8 csim_data[CSIM_DATA_LEN];

#endif
