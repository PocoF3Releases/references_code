/************************************************************************/
/* Copyright 2014-2017 NXP Semiconductors                               */
/* Copyright 2020 GOODIX                                                */
/*                                                                      */
/* GOODIX Confidential. This software is owned or controlled by GOODIX  */
/* and may only be used strictly in accordance with the applicable      */
/* license terms.  By expressly accepting such terms or by downloading, */
/* installing, activating and/or otherwise using the software, you are  */
/* agreeing that you have read, and that you agree to comply with and   */
/* are bound by, such license terms.                                    */
/* If you do not agree to be bound by the applicable license terms,     */
/* then you may not retain, install, activate or otherwise use the      */
/* software.                                                            */
/*                                                                      */
/************************************************************************/

#ifndef TFA_HAL_PLUGIN_H
#define TFA_HAL_PLUGIN_H     1


/* Here all plugins delivered with HAL defines it's interface to top layers */

#if !defined(WIN32) && !defined(_X64)  /* Linux/Unix default */

/* Sysfs plugin */
extern struct tfa_hal_plugin_funcs sysfs_plug_symbol;
extern struct tfa_hal_plugin_funcs dummy_plug_symbol;
extern struct tfa_hal_plugin_funcs hid_plug_symbol;
extern struct tfa_hal_plugin_funcs rwe_plug_symbol;
extern struct tfa_hal_plugin_funcs acpi_plug_symbol;
extern struct tfa_hal_plugin_funcs tiberius_plug_symbol; /* Only Linux */
extern struct tfa_hal_plugin_funcs tcpip_plug_symbol;
extern struct tfa_hal_plugin_funcs udp_plug_symbol;
extern struct tfa_hal_plugin_funcs i2c_plug_symbol;

#else  /* Definitions for Windows */

/* For windows some reason dll symbols have to be imported too */

/* Sysfs plugin */
__declspec(dllimport) extern struct tfa_hal_plugin_funcs dummy_plug_symbol;
__declspec(dllimport) extern struct tfa_hal_plugin_funcs hid_plug_symbol;
__declspec(dllimport) extern struct tfa_hal_plugin_funcs tfausb_plug_symbol;
__declspec(dllimport) extern struct tfa_hal_plugin_funcs rwe_plug_symbol;
__declspec(dllimport) extern struct tfa_hal_plugin_funcs acpi_plug_symbol;
__declspec(dllimport) extern struct tfa_hal_plugin_funcs tcpip_plug_symbol;
__declspec(dllimport) extern struct tfa_hal_plugin_funcs udp_plug_symbol;

#endif /* !defined(WIN32) && !defined(_X64) */
#endif /* TFA_HAL_PLUGIN_H */
