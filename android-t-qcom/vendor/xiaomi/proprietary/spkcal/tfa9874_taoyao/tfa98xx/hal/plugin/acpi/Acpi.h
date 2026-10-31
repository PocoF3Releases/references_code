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

#ifndef ACPI_H_
#define ACPI_H_
#include "windows.h"

typedef void(__stdcall *lpOut32)(short, short);
typedef short(__stdcall *lpInp32)(short);
typedef BOOL(__stdcall *lpIsInpOutDriverOpen)(void);
typedef BOOL(__stdcall *lpIsXP64Bit)(void);

typedef struct AcpiContext
{
	HINSTANCE hInpOutDll;
	lpOut32 gfpOut32;
	lpInp32 gfpInp32;
	lpIsInpOutDriverOpen gfpIsInpOutDriverOpen;
	lpIsXP64Bit gfpIsXP64Bit;

}AcpiContext;
//int Acpi_LoadInOutPortDLL(void);
int Acpi_DeInit(AcpiContext *pAcpiContext);
int Acpi_Init(AcpiContext **ppAcpiContext);

byte ReadByte(byte reg);
void WriteByte(byte reg, byte value);
byte SetBurstMode(BOOL enable);


#endif
