#ifndef MI_RAW_PROBE_NFC_INITIATOR_H
#define MI_RAW_PROBE_NFC_INITIATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


int mf_nfc_mode_configure(int bRawMode);
int mf_transceive_bytes(const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx);
int mf_transceive_bits(const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar, uint8_t *pbtRx, uint8_t *pbtRxPar);

#ifdef __cplusplus
}
#endif


#endif

