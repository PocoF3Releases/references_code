/*
 * system.h   TFA calibration tool
 *
 * Copyright 2014-2018 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "cutils/log.h"


#define AUDIO_PLAYER "/system/bin/tinyplay"
#define AUDIO_MIXER "/system/bin/tinymix"

#define SYS_EXEC_FAILED	"failed"
#define SYS_EXEC_ERR	"Err"
#define SYS_EXEC_err	"err"


// Hypothetical System Functions
extern pid_t sys_play_wav(char * pFile, char * pMode);
extern void sys_stop_wav(pid_t nProcess);
extern void sys_delay(uint32_t delay_ms);
extern double sys_get_ambient_temp(void);
extern int sys_is_valid(char * pFile);

extern int sys_exec(char *cmd,char *outputstream);
extern void sys_mixer_command(char *pCommand, int nData);
#endif /* SYSTEM_H_ */
