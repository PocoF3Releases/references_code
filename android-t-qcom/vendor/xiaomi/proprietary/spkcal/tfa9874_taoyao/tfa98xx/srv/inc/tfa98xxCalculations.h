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

#ifndef TFA98XXCALCULATIONS_H_
#define TFA98XXCALCULATIONS_H_

#ifndef int24_t
#define int24_t int
#endif

void windowing_filter(double *data, int L);
void fft_abs(const double *signal, double * absH, int fft_length);
void freqz_abs(const double *b, int b_length, const double *a, int a_length, double *absH, int fft_length);
void leakageFilter(double *data, int cnt, double lf);
void compute_Zf(double *wY, double *absZ, int fft_size);
void compute_Xf(double *wY, double *absX, double *aw, double *scratch, int lagW,
        double fs, double leakageFactor, double ReCorrection, double Bl, double ReZ, int fft_size);

/* read the current status of the DSP, typically used for development, */
/* not essential to be used in a product                               */
enum Tfa98xx_Error tfa98xx_calculate_state_info(
                                const unsigned char bytes[],
				struct Tfa98xx_StateInfo *pInfo);

/* For SmartStudio display, need to convert lines into DB values ,       */
/* as is done in Matlab for graph display.                               */
double lin2db(int fac);

#define FFT_SIZE	128
#define MAX_NR_SUBBANDS 6
#define FFT_SIZE_DRC	1024    // to be determined based on the GUI resolution and the available MIPS
#define GUI_LOW_LIMIT	(-30.0) // this determines the lower range on what is plotted in the GUI for the input filter transfer function
// GUI parameters
#define DRC_GUI_LOW_LIMIT_DB   (-80.0) // this determines the lowest I/O range in dB of what is plotted in the GUI.
#define DRC_GUI_HIGH_LIMIT_DB  (0.0)   // this determines the highest I/O range in dB of what is plotted in the GUI.
#define DRC_GUI_PLOT_LENGTH    (int)(DRC_GUI_HIGH_LIMIT_DB - DRC_GUI_LOW_LIMIT_DB + 1) // ploted points at 1 dB increment

// DSP based x model constants
#define FFT_SIZE_XMODEL_DSP (128u)
#define BQ_SCALE            (double)8388608
#define BQ_M_SCALE          (double)-8388608
/* The HostSDK reports the frequency response of the X model as
if the sample frequency is SAMPLE_RATE_XMODEL_DISPLAY even if the 
internal sample rate is different. This will be corrected in SMART-949 */
#define SAMPLE_RATE_XMODEL_DISPLAY (8000)

#define FFT_SIZE_ZMODEL_DSP (128u)
#define Z_SCALE             ((double)(1 << 22))


// Biquad filter coefficients in the order
// returned by the ITF GetMBDrcDynamics command
typedef struct s_biquad_t
{
	int24_t scale; // Coefficients scale factor
	double b2;
	double b1;
	double b0;
	double a2;
	double a1;
}s_biquad_t;

/** Order of the coefficients in the ITF of the biquad of EQ
This is an alternative description of tfaBiquad_t but with the
headroom (H) parameter included.*/
#define BIQUAD_COEFFS_H_IDX            (0u)
#define BIQUAD_COEFFS_A1_IDX           (2u)
#define BIQUAD_COEFFS_A2_IDX           (1u)
#define BIQUAD_COEFFS_B0_IDX           (5u)
#define BIQUAD_COEFFS_B1_IDX           (4u)
#define BIQUAD_COEFFS_B2_IDX           (3u)

#define BIQUAD_COEFFS_SIZE (6u) /** Number of values in Biquad Coeffs */
#define NBR_OF_COEFFS_BQ (3u) /** 3 values in B or A of a biquad */

/** Structure to store the coefficients of a generic Biquad
This is a more generic alternative to coeffs_float_t that does not require a0
to be equal to 1.*/
typedef struct BQ_coeffs_float {
    double B[NBR_OF_COEFFS_BQ]; /**< Coefficients of the numerator */
    double A[NBR_OF_COEFFS_BQ]; /**< Coefficients of the denominator */
} BQ_coeffs_float_t;



// Gains & DRCs envelopes structure returned by GetMBDrcDynamics
// after the fixed to float scaling is applied
typedef struct s_subBandGains
{
	double subBandGain;
	double DRC1gain;       // if DRC2 is disabled, DRC1gain = subBandGain
	double DRC1envelopeDb; // DRC1 envelope in dB = subband input signal envelope   //25
	double DRC2gain;
	double DRC2envelope;   // DRC2 envelope in dB
}s_subBandGains;

// Sub Band Information
typedef struct s_subBandDynamics
{
  s_biquad_t      BQ_1;   // input filter coeffs
  s_biquad_t      BQ_2;   // gain filter coeffs
  s_subBandGains  Gains;
}s_subBandDynamics;


// DRC types, with the same indexing as the GUI produces
typedef enum {
  DRC_TYPE_LOG_SMP_COMP=0,     /* 0 */
  DRC_TYPE_LOG_BLK_PEAK_COMP,  /* 1 */
  DRC_TYPE_LOG_BLK_AVG_COMP,   /* 2 */
  DRC_TYPE_LIN_BLK_XPN,        /* 3 */
  DRC_NB_OF_TYPES
} DRC_Type;

typedef struct
{
  int24_t    on;
  DRC_Type   type;              //<   processing type
  double     thresdB;
  double     ratio;             //< ratio
  double     tAttack;           //< 0.1ms
  double     tHold;             //< 0.1ms
  double     tRelease;          //< 0.1ms
  double     makeUpGaindB;
  double     kneePercentage;    //< 0 - 100
} s_DRC_Params;

int mbdrc_get_index_activated_subband(int subband_selector, int n);
void tfa98xx_calculate_dynamix_variables_drc(int *data, s_subBandDynamics *sbDynamics, int index_subband);
void compute_input_filter_transf_function(double *outFreqTrFunc, s_biquad_t *bqCoef, double *scratch, int fft_size, double lowLimit);
void compute_gain_filter_transf_function(double *outFreqTrFunc, s_biquad_t *bqCoef, double subBandGain, double *scratch, int fft_size, int type);
void compute_gain_combined_transf_function(double *outFreqTrFunc, double subBandsTrFunc[][FFT_SIZE_DRC/2], int nr_subbands, int buffLength);
int bitCount_subbands(int index_subband);
void compute_drc_transf_function(double *transfFunctDb, s_DRC_Params *paramsDRC, double *envelopeDb, int length);
void compute_drc_combined_transf_function(double *combinedTransfFunctDb, double transfFunctDb[2][DRC_GUI_PLOT_LENGTH], double *envelopeDb, int length);

// DSP x-model
void complex_mod(double *a_real, double *a_imag, double *hX, int L);
void apply_biquad(double *hX, double *exc_filter, int pos_filt, int exp_coef, double resample);

/**
 *  Compute the speaker x-model and return an array of values
 *  @param excursion_filter
 *  @param data_x_coef
 *  @param fs
 *  @param xModel_fs
 *  @param waveform pointer to first element of a waveform data array (double waveform[128])
 */
void compute_x_model_values(int *excursion_filter, int *data_x_coef,
                            int fs, int xModel_fs,
                            double *waveform);

/**
 * Compute the parametric x filter frequency response
 * @param coeffs array of parametric x filter coefficients
 * @param sampleRate of the DSP
 * @param waveform pointer to first element of a frequency response
 */
void compute_parametric_x_filter_values(const int *coeffs, int sampleRate, double waveform[FFT_SIZE_XMODEL_DSP]);

/**
 * Compute the parametric z filter frequency response
 * @param coeffs array of parametric z filter coefficients
 * @param waveform pointer to first element of a frequency response
 */
void compute_parametric_z_filter_values(const int *coeffs, double waveform[FFT_SIZE_ZMODEL_DSP]);

void translate_bq_coeffs_to_floating_point(const int fixed_values[BIQUAD_COEFFS_SIZE], BQ_coeffs_float_t* float_values);

int tfa_24_to_32(int32_t * data32, unsigned char *data24, int length_bytes24);

#endif                          /* TFA98XXLIFEDATA_H_ */
