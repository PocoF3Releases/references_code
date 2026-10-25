/*
 * Copyright (C) 2015-2020 China IWNCOMM Co., Ltd. All rights reserved.
 *
 * Contains data structures and function declarations for wapi
 *
 * Authors:
 * iwncomm
 *
 * History:
 * yucun tian  09/10/2015 v1.0 maintain the code.
 * yucun tian  07/20/2016 v1.1 fix for change code style.
 *
 * Related documents:
 * -GB/T 15629.11-2003/XG1-2006
 * -ISO/IEC 8802.11:1999,MOD
 *
*/

#ifndef _WAPI_ASUE_H_
#define _WAPI_ASUE_H_


#include "wai_common.h"
#include "wapi_cert_if.h"

#define BK_LEN 16

#define PAGE_LEN	4096
#define BKID_LEN	16
#define CHALLENGE_LEN	32
#define PAIRKEY_LEN	16
#define USKSA_LEN	(4 * PAIRKEY_LEN + CHALLENGE_LEN)
#define ADDID_LEN	(ETH_ALEN * 2)
#define WAI_MIC_LEN	20
#define WAI_IV_LEN	16
#define WAI_KEY_AN_ID_LEN	16
#define ISUSK		1
#define WAI_HDR	12
/*ECDSA192+SHA256*/
#define WAPI_ECDSA_OID          "1.2.156.11235.1.1.1"
/*CURVE OID*/
#define WAPI_ECC_CURVE_OID      "1.2.156.11235.1.1.2.1"
#define ECDSA_ECDH_OID          "1.2.840.10045.2.1"

#define WAPI_OID_NUMBER     1
#define MAX_BYTE_DATA_LEN   256
/* #define COMM_DATA_LEN       2048 */

#define PACK_ERROR  0xffff
#define PUBKEY_LEN             		48
#define PUBKEY2_LEN             		49
#define SECKEY_LEN             		24
#define DIGEST_LEN             		32
#define HMAC_LEN                        	20
#define PRF_OUTKEY_LEN			48
#define SIGN_LEN               	48
#define KD_HMAC_OUTKEY_LEN	     96
#define IWN_FIELD_OFFSET(type, field)   ((int)(&((type *)0)->field))
#define X509_TIME_LEN       15
#define CERT_OBJ_NONE 0
#define CERT_OBJ_X509 1

#define MSK_TEXT "multicast or station key expansion for station unicast and multicast and broadcast"
#define USK_TEXT "pairwise key expansion for unicast and additional keys and nonce"


enum wapi_states {
	/**
	 * WAPI_DISCONNECTED - Disconnected state
	 */
	WAPI_DISCONNECTED = 0,

	/**
	 * WAPI_ASSOCIATED - Association completed
	 */
	WAPI_ASSOCIATED = 1,

	/**
	 * WAPI_CERT_HANDSHAKE - Certificate handshake in process
	 */
	WAPI_CERT_HANDSHAKE = 2,

	/**
	 * WAPI_CERT_COMPLETED - Certificate handshake completed
	 */
	WAPI_CERT_COMPLETED = 3,

	/**
	 * WAPI_UNICAST_KEY_CHECK - Unicast key negotication check wai frame
	 */
	WAPI_UNICAST_KEY_CHECK = 4,

	/**
	 * WAPI_UNICAST_KEY_HANDSHAKE - Unicast key negotiation in process
	 */
	WAPI_UNICAST_KEY_HANDSHAKE = 5,

	/**
	 * WAPI_UNICAST_KEY_COMPLETED - Unicast key negotiation completed
	 */
	WAPI_UNICAST_KEY_COMPLETED = 6,

	/**
	 * WAPI_MULTICAST_KEY_HANDSHAKE - Multicast key negotiation in process
	 */
	WAPI_MULTICAST_KEY_HANDSHAKE = 7,

	/**
	 * WAPI_COMPLETED - All authentication completed
	 */
	WAPI_COMPLETED = 8,
};


struct wai_hdr
{
	u8 version[2];
	u8 type;
	u8 stype;
	u8 reserve[2];
	u16 length;
	u8 rxseq[2];
	u8 frag_sc;
	u8 more_frag;
	/* followed octets of data */
};

struct wapi_bksa_cache
{
	u8 bkid[BKID_LEN];
	u8 bk[BK_LEN];
	u8 asue_auth_flag[32];
	size_t bk_len;
	/* WAPI_KEY_MGMT */
	int akmp;
	u8 ae_mac[ETH_ALEN];
	u8 pad[2];
};

struct wapi_usk
{
	/* Unicast Encryption Key */
	u8 uek[PAIRKEY_LEN];
	/* Unicast Integrity check Key (UCK) */
	u8 uck[PAIRKEY_LEN];
	/* Message Authentication Key (MAK)*/
	u8 mak[PAIRKEY_LEN];
	/*Key Encryption Key (KEK) */
	u8 kek[PAIRKEY_LEN];
	u8 ae_challenge[CHALLENGE_LEN];
	u8 asue_challenge[CHALLENGE_LEN];
};
struct wapi_usksa
{
	u8 uskid;
	u8 usk_pad[3];
	struct wapi_usk usk[2];
	int ucast_suite;
	u8 ae_mac[ETH_ALEN];
	u8 mac_pad[2];
};

struct wapi_msksa
{
	u8 direction;
	u8 mskid;
	u8 msk_pad[2];
	u8 msk[32];
	u8 msk_ann_id[16];
	int ucast_suite;
	u8 ae_mac[ETH_ALEN];
	u8 mac_pad[2];
};

struct wapi_sm_ctx {
	void *ctx; /* pointer to arbitrary upper level context */
	void *msg_ctx; /* upper level context for wpa_msg() calls */

	void (*set_state)(void *ctx, enum wapi_states state);
	enum wapi_states (*get_state)(void *ctx);
	void (*deauthenticate)(void *ctx);
	int (*set_multicast_key)(void *ctx, const unsigned char* pKeyValue, int keylength, int key_idx, const unsigned char* keyIV);
	int (*set_unicast_key)(void *ctx, const char* pKeyValue, int keylength, int key_idx);
	int (*get_bssid)(void *ctx, unsigned char* bssid);
	int (*get_own_addr)(void *ctx, unsigned char* own_addr);
	wapi_cert_data* (*get_cert_info)(void *ctx);
	int (*get_ap_type)(void *ctx);
	void (*auth_failed)(void *ctx);
};

enum wapi_sm_conf_params
{
	WAPI_PARAM_AP_TYPE = 0,
};


struct wapi_rxfrag
{
	const u8 *data;
	int data_len;
	int maxlen;
};

typedef struct byte_data_
{
	u8 length;
	u8 pad[3];
	u8 data[256];
}byte_data;

struct wpa_certs
{
	int type;
	int status;
	byte_data *serial_no;
	byte_data *as_name;
	byte_data *user_name;
};


struct cert_bin_t
{
	unsigned short length;
	unsigned short pad;
	unsigned char *data;
};

typedef struct _wai_fixdata_cert
{
	u16 cert_flag;
	u16 pad;
	struct cert_bin_t cert_bin;
} wai_fixdata_cert;

typedef struct _para_alg
{
	u8  para_flag;
	u16 para_len;
	u8  pad;
	u8  para_data[256];
}para_alg, *ppara_alg;


/*signature algorithm*/
typedef struct _wai_fixdata_alg
{
	u16 alg_length;
	u8  sha256_flag;
	u8  sign_alg;
	para_alg sign_para;
}wai_fixdata_alg;

typedef struct _resendbuf_st
{
	u16 cur_count;
	u16 len;
	void *data;
}resendbuf_st;


struct wapi_sm
{
	struct wapi_sm_ctx *ctx;
	struct wapi_usksa usksa;
	struct wapi_msksa msksa;
	/* PMKSA cache */
	struct wapi_bksa_cache bksa;

	struct wapi_rxfrag *rxfrag;
	u16 rxseq;
	u16 txseq;

	/* Own WAPI/RSN IE from (Re)AssocReq */
	u8 assoc_wapi_ie[256];
	u8 assoc_wapi_ie_len;

	u8 ap_wapi_ie[256];
	u8 ap_wapi_ie_len;

	u8 ae_auth_flag[32];
	u8 Nasue[32];
	u8 Nae[32];
	wai_fixdata_id ae_asu_id;
	wai_fixdata_id ae_id;
	wai_fixdata_id asue_id;
	wai_fixdata_alg sign_alg;
	byte_data asue_key_data;
	/* ASUE temp private key */
	byte_data asue_eck;
	byte_data ae_key_data;
	para_alg ecdh;

	cert_id asue_cert;
	cert_id ae_cert;

	u8 flag;
	u8 ukey_flag;
	/* flag for first waigroup_unicast,1:yes,0:no*/
	int usk_first_auth_flag;
	/* resend flag of AP auth_active frame  1 :yes  0: no*/
	int auth_active_retry;
	/*BK update flag  1:yes 0 no*/
	int bk_update_flag;

	int usk_updated;
};


//#define TIMER_DATA_MAXLEN    8192
#define WAPI_TX_BUF_SIZE	8192

struct timer_dispose
{
	/* type, 0-none, 1-wait assoc, 2-wait unicast, 3-wait finish*/
	int t;
	int ticks;
	/*count of tx*/
	int txs;
	int l;
	u8 dat[WAPI_TX_BUF_SIZE];
};

struct wapi_asue_st
{
	void* ctx;
	u8 own_addr[ETH_ALEN];
	u8 own_addr_pad[2];
	u8 bssid[ETH_ALEN];
	u8 ssid_pad[2];
	/* reassociation requested */
	int reassociate;
	int disconnected;
	int wai_received;
	/* Selected configuration (based on Beacon/ProbeResp WPA IE) */
	int pairwise_cipher;
	int group_cipher;
	int key_mgmt;
	struct wapi_sm *wapi;
	enum wapi_states wapi_state;
	u8 last_wai_src[ETH_ALEN];

	wapi_cert_data cert_info;

	struct timer_dispose* ptm;
	struct timer_dispose vpt_tm;

	/*0-open,1-cert,2-key*/
	int ap_type;
	u8 psk_bk[16];


};

#define WAI_VERSION 1
#define WAI_TYPE 1
#define WAI_FLAG_LEN 1

/* WAI packets type */
enum
{
	/*pre-authentication start*/
	WAI_PREAUTH_START = 0x01,
	/*STAKey request */
	WAI_STAKEY_REQUEST = 0x02,
	/*authentication activation*/
	WAI_AUTHACTIVE	= 0x03,
	/*access authentication request */
	WAI_ACCESS_AUTH_REQUEST = 0x04,
	/*access authentication response */
	WAI_ACCESS_AUTH_RESPONSE = 0x05,
	/*certificate authentication request */
	WAI_CERT_AUTH_REQUEST = 0x06,
	/*certificate authentication response */
	WAI_CERT_AUTH_RESPONSE = 0x07,
	/*unicast key negotiation request */
	WAI_USK_NEGOTIATION_REQUEST = 0x08,
	/* unicast key negotiation response */
	WAI_USK_NEGOTIATION_RESPONSE = 0x09,
	/*unicast key negotiation confirmation */
	WAI_USK_NEGOTIATION_CONFIRMATION = 0x0A,
	/*multicast key/STAKey announcement */
	WAI_MSK_ANNOUNCEMENT = 0x0B,
	/*multicast key/STAKey announcement response */
	WAI_MSK_ANNOUNCEMENT_RESPONSE = 0x0C,
	//WAI_SENDBK = 0x10, /*BK  for TMC ??*/
};

/* Timer */
void wapi_timer_set(int t, const u8* dat, int l);
void wapi_timer_reset(void);
void wapi_timer_destory(void);
void wapi_try_resend(void);

struct wapi_rxfrag  *malloc_rxfrag(int maxlen);
void *free_rxfrag(struct wapi_rxfrag *frag);

int wapi_asue_tx_wai(void *ctx, const u8 *buf, size_t len);
int iwn_wai_fixdata_id_by_ident(void *cert_st,
				wai_fixdata_id *fixdata_id,
				u16 index);
void wapi_sm_rx_wai(struct wapi_sm *sm,
				const unsigned char *src_addr,
				const unsigned char *buf, size_t len);

struct wapi_sm* wapi_sm_init(struct wapi_sm_ctx *ctx);
void wapi_sm_deinit(struct wapi_sm *sm);

#endif
