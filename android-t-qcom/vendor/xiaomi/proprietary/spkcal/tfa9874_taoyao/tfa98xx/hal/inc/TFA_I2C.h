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

/** * @defgroup HAL target interface abstraction (TFA_I2C hal)
 * @{
*/

#ifndef TFA_I2C_H
#define TFA_I2C_H

/* The maximum I2C message size allowed for read and write buffers, incl the slave address */
#define TFA_I2C_MAX_SIZE 254 // TODO remove this'TFA_I2C_MAX_SIZE, its platform value

#ifdef __cplusplus
extern "C" {
#endif

/* A glue layer.
 * The hostsdk components will use the functions defined in this API to do the actual calls to I2C
 * Make sure you implement this to use your I2C access functions (which are SoC and OS dependent)
 */

/**
 * A list of supported I2C errors that can be returned.
 */
enum TFA_I2C_Error {
	TFA_I2C_UnassignedErrorCode,		/**< UnassignedErrorCode*/
	TFA_I2C_Ok,				/**< no error */
	TFA_I2C_NoAck, 		/**< no I2C Ack */
	TFA_I2C_TimeOut,		/**< bus timeout */
	TFA_I2C_ArbLost,		/**< bus arbritration lost*/
	TFA_I2C_NoInit,		/**< init was not done */
	TFA_I2C_UnsupportedValue,		/**< UnsupportedValu*/
	TFA_I2C_UnsupportedType,		/**< UnsupportedType*/
	TFA_I2C_NoInterfaceFound,		/**< NoInterfaceFound*/
	TFA_I2C_NoPortnumber,			/**< NoPortnumber*/
	TFA_I2C_BufferOverRun,			/**< BufferOverRun*/
	TFA_I2C_ErrorMaxValue				/**<impossible value, max enum */
};

typedef enum TFA_I2C_Error TFA_I2C_Error_t;

/**
 * Hal interface flags
 */
enum TFA_I2C_IfType {
	TFA_I2C_Unknown=0, 	/**< no type defined */
	TFA_I2C_Direct=1, 	/**< no bus interface */
	TFA_I2C_I2C=2, 		/**< i2c hardware interface */
	TFA_I2C_HID=4, 		/**< usb hid interface */
	TFA_I2C_Msg=8, 		/**< message capable interface */
	TFA_I2C_Playback=0x10, /**< implement audio playback */
	TFA_I2C_IfTypeMaxVale=0xff /**< max enum */
};
typedef enum TFA_I2C_IfType TFA_I2C_IfType_t;

/**
 * Open and register a target interface.
 *
 *  @param targetname interfacename
 *  @return filedescripter (if relevant)
 */
int TFA_I2C_Open(char *targetname);

/**
 * Close and un-register the target interface.
 *
 *  Note that the target may block the caller when it has not been properly
 *  shutdown.
 */
int TFA_I2C_Close(void);

/**
 *  Returns the maximum number of bytes that can be transfered in one burst transaction.
 *
 *  @return max burst size in bytes
 */
int TFA_I2C_BufferSize(void);

/**
 *  Pass through function to get the scribo name
 *
 *  @param buf = char buffer to write the name
 *  @param count = the maximum size required
 */
void TFA_I2C_Scribo_Name(char *buf, int count);

/**
 *  Return the interface type
 *  @return TFA_I2C_IfType_t enum
 */
TFA_I2C_IfType_t TFA_I2C_Interface_Type(void);

/**
 * Execute a write, followed by I2C restart and a read of num_read_bytes bytes.
   The read_buffer must be big enough to contain num_read_bytes.
 *
 *  @param sla = slave address
 *  @param num_write_bytes = size of data[]
 *  @param write_data[] = byte array of data to write
 *  @param num_read_bytes = size of read_buffer[] and number of bytes to read
 *  @param read_buffer[] = byte array to receive the read data
 *  @return TFA_I2C_Error_t enum
 */
TFA_I2C_Error_t TFA_I2C_WriteRead(  unsigned char sla,
				int num_write_bytes,
				const unsigned char write_data[],
				int num_read_bytes,
				unsigned char read_buffer[] );

/**
 * Enable/disable I2C transaction trace.
 *
 *  @param on = 1, off = 0
 *
 */
void TFA_I2C_Trace(int on);
void tfa_UDP_Trace(int on);

/**
 * Use tracefile to write trace output.
 *
 *  @param filename: 0=stdout,  "name"=filename, -1=close file
 *  @return filedescripter or -1 on error
 */
void TFA_I2C_Trace_file(char *filename);

/**
 * Read back the pin state.
 *
 *  @param pin number
 *  @return pin state
 */
int TFA_I2C_GetPin(int pin);

/**
 * Set the pin state.
 *
 *  @param pin number
 *  @param value to set
 *  @return TFA_I2C_Error_t enum
 */
int TFA_I2C_SetPin(int pin, int value);

/**
 * Read back version info as a string.
 *
 *  @param data string buffer to hold the string (will not exceed 1024 bytes)
 *  @return TFA_I2C_Error_t enum
 */
TFA_I2C_Error_t TFA_I2C_Version(char *data);

/**
 * Stop playback
 *
 *  @return TFA_I2C_Error_t enum
 */
TFA_I2C_Error_t TFA_I2C_StopPlayback(void);

/**
 * Start playback of named audiofile
 *
 *  @param data audio file name, max 255 chars.
 *  @return TFA_I2C_Error_t enum
 */
TFA_I2C_Error_t TFA_I2C_StartPlayback(char *data);

/**
 * Return the string for the error.
 *
 *  @param  error code
 *  @return string describing TFA_I2C_Error_t enum
 */
const char *TFA_I2C_Error_string(TFA_I2C_Error_t error);

/**
 * @brief Return the interface type.
 *
 *  @return  TFA_I2C_IfType_t enum
 */
TFA_I2C_IfType_t TFA_I2C_Interface_Type(void);

/**
 * @brief Execute a command on the tfadsp
 *
 * This function is the tfadsp direct DSP algorythm framework interface which
 * facilitates sending and receiving of RPC messages.\n
 * The command in the buffer will be executed and the response will be returned.
 * The underlying command and result operations can be separated in write only
 * and read only by specifying zero length for the corresponding action.
 * For the result read this means that it relies on a command only call that has
 * executed before the result call is done with zero command length.
 *
 *   -  <b>command_length!=0 and result_length==0 </b>\n
 *   				execute command in command buffer\n
 *   				return value is result[0]/tfadsp status or <0 if error
 *   -  <b> command_length!=0 and result_length!=0 </b>\n
 *   				execute command in command buffer\n
 *   				read result to return buffer\n
 *   				return value is actual returned length or <0 if error
 *   -  <b> command_length==0 and result_length!=0 </b>\n
 *   				read result to return buffer\n
 *   				return value is actual returned length or <0 if error
 *
 * In case the result_length is non-zero a result of maximum this length will be returned.
 * In case of zero the 1st word of the result will be returned as the call status.
 *
 *
 *  @param slave address of the target
 *  @param command_length command buffer length in bytes
 *  @param command_buffer pointer to the command buffer
 *  @param result_length result buffer length in bytes
 *  @param result_buffer pointer to the result buffer
 *
 *  @return if <0 error
 *  @return if 0 success in single command
 *  @return if > 0 tfadsp status if result_length==0
 *  @return if > 0 return size if result_length!=0
 *
 *
 */
TFA_I2C_Error_t TFA_TFADSP_Execute(int slave, int command_length, void *command_buffer,
					 int result_length, void *result_buffer);
/**
 * Call remote tfadsp_init()
 *
 */
TFA_I2C_Error_t TFA_TFADSP_Init(int slave, void *buffer, int length);
/** @} */

/**
 * Allows to retrive hal intefrace form TFA_I2C layer in case of TFA_I2C_Open call
 *
 * @pre be sure TFA_I2C_Open() was called.
 *
 * @return handle to hal plugin or NULL
 *
 */
TFA_I2C_Error_t hdsk_I2C_GetHal(void **hal);

#ifdef __cplusplus
}
#endif

#endif // TFA_I2C_H
