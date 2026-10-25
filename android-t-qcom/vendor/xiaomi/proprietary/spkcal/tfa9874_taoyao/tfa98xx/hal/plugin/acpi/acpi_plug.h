/*
 * Copyright 2019 NXP Semiconductors
 * NOT A CONTRIBUTION.
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

#ifndef ACPI_PLUG_H_
#define ACPI_PLUG_H_

#define I2C_READ_CMD		0x01
#define I2C_WRITE_CMD		0x02
#define I2C_WRITE_READ		0x03

#define	AMP_BOTTOM_LEFT		0x01
#define	AMP_BOTTOM_RIGHT	0x02
#define	AMP_TOP_LEFT		0x03
#define AMP_TOP_RIGHT		0x04

#define	CMD_EXE_STATUS		0x00
#define	SET_PAGE_NUM		0x10
#define CLR_ALL_CMD			0x00

/* EC offset list */

#define	OFSET_INDEX				0x00	/*Command Execution Status*/
#define	OFSET_RW_SET			0x01	/*I2C Read or Write*/
#define	OFSET_TARGET_AMP_ID		0x02	/*Target Amplifier ID*/
#define	OFSET_REG_ADD			0x03	/*Register Address*/
#define	OFSET_NUM_BYTES			0x04	/*Number of bytes to write or read*/
#define	OFSET_DATA_START		0x05	/*Data to write or read start point*/
#define	OFSET_DATA_END			0xFE	/*Data to write or read end point*/
#define OFSET_DISCRETE_PAGE		0xFF	/*Discrete amp related pages*/

#define WRITE_BYTE_LEN			15
#define WRITE_READ_BYTE_LEN		10

#define SEEK_VALUE_INDEX	6
#define SEEK_VALUE_NEXT		9

static int write_read(unsigned int out_len);
void frame_write_msg_batch(FILE *fp, unsigned char offset, unsigned char data);
static int is_amp_active(unsigned char offset, unsigned char data);

#endif
