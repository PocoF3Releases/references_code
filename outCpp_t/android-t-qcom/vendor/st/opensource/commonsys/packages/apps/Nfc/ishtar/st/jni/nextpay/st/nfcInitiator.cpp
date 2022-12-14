#include "nfcInitiator.h"
#include "StRawJni.h"

int mf_transceive_bytes(const uint8_t *pbtTx, const size_t szTx, uint8_t *pbtRx) {
	return nfc_initiator_transceive_bytes(NULL, pbtTx, szTx, pbtRx, sizeof(pbtRx), 500);
}

int mf_transceive_bits(const uint8_t *pbtTx, const size_t szTxBits, const uint8_t *pbtTxPar, uint8_t *pbtRx, uint8_t *pbtRxPar) {
    return nfc_initiator_transceive_bits(NULL, pbtTx, szTxBits, pbtTxPar, pbtRx, sizeof(pbtRx), pbtRxPar);
}

int mf_nfc_mode_configure(int bRawMode) {
     // TEMPORARY: force to restart discovery here, since NCI_PARAM_ID_PROP_POLL_TX_TICK_INTERVAL not applied after sleep transition
     return nfc_mode_configure(1, 1);
}
