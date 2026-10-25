//
// Created by liuguilin on 17-10-22.
//

#ifndef GRADLEPROJECT_VIRTUALSIM_H
#define GRADLEPROJECT_VIRTUALSIM_H

typedef unsigned char uint8;

#define IS_DEBUG 0

#define USIM_DATA_LEN 55
#define SIM_DATA_LEN 14
#define CSIM_DATA_LEN 18

uint8 usim_data[USIM_DATA_LEN];
uint8 sim_data[SIM_DATA_LEN];
uint8 csim_data[CSIM_DATA_LEN];

void authenticationUsim(uint8 *auth_apdu);

#endif //GRADLEPROJECT_VIRTUALSIM_H
