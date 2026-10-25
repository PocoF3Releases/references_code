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

/**
 * @file tfa98xxDiagnostics.h
 *  @brief Production diagnostics:
 *      simple exercise of the Maximus registers to ensure proper wiring and
 *      accessibility of the functionality of Maximus.
 *
 *     @brief  The following tests are available:
 *
 *              - test 0 : run all tests
 *              - test 1 : scan of I2C for ID registers and check for expected device
 *              - test 2 : write/read - test of a register
 *              - test 3 : check status register flags and assume coldstart (or fail)
 *              - test 4 : verify default state of relevant registers
 *              - test 5 : enable clocks in bypass and verify status
 *              - test 6 : start DSP and verify (by reading ROM tag and check status)
 *              - test 7 : load configuration settings and verify readback
 *              - test 8 : load preset values and verify readback
 *              - test 9 : load speaker parameters and verify readback
 *              - test 10 : check battery level
 *              - test 11 : verify speaker presence by checking the resistance
 *              - test 12 : assume I2S input and verify signal activity
 *
 */

#ifndef TFA98XXDIAGNOSTICS_H_
#define TFA98XXDIAGNOSTICS_H_

/*
 * the  following is directly from tfaRuntime
 *  diag should avoid external type dependency
 */
void tfaRun_Sleepus(int us);

#define TFA_DIAG_REV_MAJOR    (1)         // major API rev
#define TFA_DIAG_REV_MINOR    (0)         // minor

extern int tfa98xxDiag_trace;

#define DIAGTRACE

/**
 * Diagnostic test description structure.
 */
enum tfa_diag_group {
	tfa_diag_all, 		/**< all tests, container needed, destructive */
	tfa_diag_i2c,	 	/**< I2C register writes, no container needed, destructive */
	tfa_diag_dsp, 	/**< dsp interaction, container needed, destructive */
	tfa_diag_sb, 		/**< SpeakerBoost interaction, container needed, not destructive */
	tfa_diag_func, 	/**< functional tests,  destructive */
	tfa_diag_pins  	/**< Pin tests, no container needed, destructive */
};
typedef struct tfa98xxDiagTest {
        int (*function) (int);          /**< The function pointer of the test */
        const char *description;/**< single line description */
        enum tfa_diag_group group;	/**< test group : I2C ,DSP, SB, PINS */
} tfa98xxDiagTest_t;

void tfa98xxDiagSetProfile(int profile);

int check_bitfield_settings(int dev_idx);

/**
 * container needs to be loaded
 * @param nr test number
 * @return 1 if needed
 */
int tfa_diag_need_container(int nr) ;

/**
 * run a testnumber
 *
 *  All the diagnostic test functions will return 0 if passed
 *  on failure the return value may contain extra info.
 * @param dev_idx to select the device index
 * @param testnumber diagnostic test number
 * @return if testnumber is too big an empty string is returned
 */
int tfa98xxDiag(int dev_idx, int testnumber);

/**
 * translate group name argument to enum
 * @param arg groupname
 * @return enum tfa_diag_group
 */
enum tfa_diag_group tfa98xxDiagGroupname(char *arg);

/**
 * run all tests in the group
 *
 *  All the diagnostic test functions will return 0 if passed
 *  on failure the return value may contain extra info.
 * @param dev_idx to select the device index
 * @param group
 * @return 0
 * @return > 0 test  failure
 * @return < 0 config data missing
 */
int tfa98xxDiagGroup(int dev_idx, enum tfa_diag_group group);

/**
 *  print supported device features
 * @param devidx to select the device index
 * @return 0
 * @return > 0 test  failure
 * @return < 0 config data missing
 */
int tfa98xxDiagPrintFeatures(int devidx);

/******************************************************************************
 *
 *    the test functions
 *
 *****************************************************************************/
/**
 * list all tests descriptions
 * @param dev_idx to select the device index
 * @return 0 passed
 */
int tfa_diag_help(int dev_idx);

/**
 * read test of DEVID register bypassing the tfa98xx API
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 */
int tfa_diag_register_read(int dev_idx);
/**
 * write/read test of  register 0x71
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 3 expected read value mismatch
 */
int tfa_diag_register_write_read(int dev_idx);
int tfa_diag_register_write_read_72(int dev_idx);
int tfa_diag_register_write_read_73(int dev_idx);
int tfa_diag_register_write_read_78(int dev_idx);

/**
 * check PLL status after poweron
 *  - powerdown
 *  - powerup
 *  - wait for system stable
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 power-on timed out
 */
int tfa_diag_clock_enable(int dev_idx);
int tfa_diag_clock_enable_72(int dev_idx);

int tfa_diag_clock_enable_73(int dev_idx);

/**
 * check PLL status after poweron applicable for tfa9874
 *  - powerdown
 *  - powerup
 *  - wait for system stable
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 power-on timed out
 */

int tfa_diag_clock_enable_74(int dev_idx);

int tfa_diag_clock_enable_78(int dev_idx);

/**
 * write/read test of  xmem locations
 * - ensure clock is on
 * - put DSP in reset
 * - write count into all xmem locations
 * - verify this count by reading back xmem
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 xmem expected read value mismatch
 */
int tfa_diag_xmem_access(int dev_idx);

/**
 * set ACS bit via iomem, read via status reg
 * - ensure clock is on
 * - clear ACS via CF_CONTROLS
 * - check ACS is clear in status reg
 * - set ACS via CF_CONTROLS
 * - check ACS is set in status reg
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 no control over ACS via iomem
 */
int tfa_diag_ACS_via_iomem(int dev_idx);

/**
 * write xmem, read with i2c burst
 * - ensure clock is on
 * - put DSP in reset
 * - write testpattern
 * - burst read testpattern
 * - verify data
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 xmem expected read value mismatch
 */
int tfa_diag_xmem_burst_read(int dev_idx);

/**
 * write/read full xmem in i2c burst
 * - ensure clock is on
 * - put DSP in reset
 * - burst write testpattern
 * - burst read testpattern
 * - verify data
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 xmem expected read value mismatch
 */
int tfa_diag_xmem_burst_write(int dev_idx);

/**
 * verify dsp response to reset toggle
 * - ensure clock is on
 * - put DSP in reset
 * - write count_boot in xmem to 1
 * - release  DSP reset
 * - verify that count_boot incremented to 2
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no proper DSP response to RST
 */
int tfa_diag_dsp_reset(int dev_idx);

/**
 * verify dsp haptic response with reset toggle
 * - ensure clock is on
 * - put DSP in reset
 * - write 1 to start obj
 * - check for 1, no response
 * - release  DSP reset
 * - write 1 to start obj
 * - verify that location returns 0xffffff, obj started
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no proper haptic response
 */
int tfa_diag_haptic_dsp_reset(int dev_idx);

/**
 * @brief A simple check of re-calculation can be done by checking:
 *  - sine-type objects with F0-tracking or boostbrake enabled: frequency value is updated
 *  - wave-type objects: the durationCntMax has changed.
 *
 * - ensure clock is on
 * - load 1st patch
 * - read full object table
 * - run hapticboost full startup
 * - re-read full object table
 * - check updates
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 hapticboost start error
 * @return 3 re-calculation values not updated or data corruption
 */
int tfa_diag_haptic_recalculation(int dev_idx);

/**
 * @brief A simple check of calibration results for valid range:
 * 	F0 : 50 .. 500
 * 	R0 :  1 ..  32
 *
 * - ensure clock is on
 * - assume hapticboost started up once
 * - read values from xmem
 * - check ranges
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 hapticboost start error
 * @return 3 MPT contents not valid
 * @return 4 values not in range
 */
int tfa_diag_haptic_f0_r0_check(int dev_idx);

/**
 * @brief Check if the object table can be read, modified and written
 *  back to the device.
 * - ensure clock is on
 * - read object table
 * - modify object table
 * - write object table back
 * - read object table back
 * - check if the table written matches the table read back
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 object table invalid
 * @return 4 object table not equal
 */
int tfa_diag_haptic_verify_objs(int dev_idx);

/**
 * @brief Load a patch and verify patch version number
 * - ensure clock is on
 * - clear xmem patch version storage
 * - load patch from container
 * - check xmem patch version storage
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 patch version no updated
 */
int tfa_diag_patch_load(int dev_idx);


/**
 * load a patch and verify patch version number
 * - ensure clock is on
 * - clear xmem patch version storage
 * - load patch from container
 * - check xmem patch version storage
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 patch version no updated
 */
int tfa_diag_patch_load(int dev_idx);


/**
 * DSP framework interaction
 */
/**
 * load the config to bring up SpeakerBoost
 * - ensure clock is on
 * - start with forced coldflag
 * - verify that the ACS flag cleared
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 DSP did not clear ACS
 */
int tfa_diag_start_speakerboost(int dev_idx);

/**
 * read back the ROM version tag
 * - ensure clock is on if I2C
 * - read ROMID TAG
 * - check 1st character to be '<'
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 no response from DSP
 * @return 4 invalid version tag format
 */
int tfa_diag_read_version_tag(int dev_idx);

/**
 * read back the API version
 * - ensure clock is on if I2C
 * - read API version
 * - check 1st character to be '<'
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 no response from DSP
 */
int tfa_diag_read_status_change(int dev_idx);
/**
 * read back the FW event and status words
 * - ensure clock is on if I2C
 * - read API version
 * - check 1st character to be '<'
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 no response from DSP
 */
int tfa_diag_read_api_version(int dev_idx);

/**
 * read back to verify that all parameters are loaded
 * - ensure clock is on
 * - assure ACS is set
 * - verify that SB parameters are correctly loaded
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 DSP is not configured
 * @return 4 speaker parameters error
 * @return 5 config parameters error
 * @return 6 preset parameters error
 * @return 7 profile pointer is wrong
 */
int tfa_diag_verify_parameters(int dev_idx);

/**
 * Verify that feature bits in MTP and cnt file are correct
 * - verify that features from MTP and cnt are equal
 * - check if feature bits match what is expected
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return -1 error in cnt file
 */
int tfa_diag_verify_features(int dev_idx);

/**
 * run a calibration and check for success
 * - ensure clock is on
 * - assure ACS is set
 * - run calibration
 * - check R for non-zero
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 DSP is not configured
 * @return 4 calibration call failed
 * @return 5 calibration failed, returned value is 0
 */
int tfa_diag_calibrate_always(int dev_idx);

/**
 * Check if the speaker error bit is set
 * - ensure clock is on
 * - calibration always is done
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 DSP is not configured
 * @return 4 calibration call failed
 * @return 5 calibration failed, returned value is 0
 */
int tfa_diag_read_speaker_status(int dev_idx);

/**
 * verify that the speaker impedance is within range
 * - ensure clock is on
 * - assure ACS is set
 * - check calibration done
 * - get Rapp from config
 * - get Rtypical from speakerfile
 * - compare result with expected value
 * Assume the speakerfile header holds the typical value of the active speaker
 * the range is +/- 15% Rtypical + Rapp
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 DSP is not configured
 * @return 4 calibration was not done
 * @return 5 calibration is not within expected range
 */
int tfa_diag_speaker_impedance(int dev_idx);

/**
 * check the interrupt hardware functionality
 * @return 0 passed
 * @return 1 no device found
 * @return 2 no clock
 * @return 3 interrupt bit(s) did not clear
 */
int tfa_diag_irq_cold(int dev_idx);
int tfa_diag_irq_warm(int dev_idx);

/**
 * scan i2c bus
 *  @return 0 if i2c devices found and otherwise 1
 */
int tfa_diag_i2c_scan(int dev_idx);

/**
 * wait for tap detection and report
 * @param dev_idx to select the device index
 * @return 0 passed
 * @return 1 cannot open device
 * @return 2 clock not running
 * @return 3 no tap profile active
 *
 *  @return error if tap detected else wait and retrun zero
 */
int tfa_diag_tap(int dev_idx);

/******************************************************************************
 *
 *    generic support functions
 *
 *****************************************************************************/

/**
 * dump all known registers
 *   returns:
 *     0 if slave can't be opened
 *     nr of registers displayed
 * @param dev_idx to select the device index
 */
int tfa98xxDiagRegisterDump(int dev_idx);


/**
 * run all tests
 * @param dev_idx to select the device index
 * @return last error code
 */
int tfa98xxDiagAll(int dev_idx);

/**
 * return the number of the test that was executed last
 * @return latest testnumber
 */
int tfa98xxDiagGetLatest(void);

/**
 * return the errorstring of the test that was executed last
 * @return last error string
 */
char *tfa98xxDiagGetLastErrorString(void);

/**
 * scan i2c bus and return the number of devices found
 * for potential tfa98xx devices, the user should check,
 * by reading r3, that a device is valid tfa98xx device
 *  @return
 *     number of i2c devices found
 */
int tfa_diag_scan_i2c_devices();

/**
 * get the i2c address of a device found during
 * tfa98xx_diag_scan_i2c_devices()
 * @param index
 * 	the index of the device to get the i2c address,
 * 	should be smaller then the number of device
 * 	returned during the scan
 *  @return
 *     the i2c address for a device found by the scan
 */
unsigned char tfa_diag_get_i2c_address(int index);

/**
 * Get the connected audio source
 */
int tfa98xxGetAudioSource(int dev_idx);
/**
 * @return 1 no device found
 * @return expected read value mismatch
 */
int tfa_diag_msg_small(int idx);
int tfa_diag_msg(int idx, int length);
int tfa_diag_msg_buffersize(int dev_idx) ;
int tfa_diag_msg_buffersizex3(int dev_idx);
int tfa_diag_msg_max_dsp(int dev_idx);
int tfa_diag_msg_max_tfadsp(int dev_idx) ;
// dump obj table
int tfa_diag_haptic_dump(int dev_idx) ;
#endif                          /* TFA98XXDIAGNOSTICS_H_ */
