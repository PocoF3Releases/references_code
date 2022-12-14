/** @file wapi_asue_i.h
 *  @brief This header file contains data structures and function declarations of wapi_asue_i
 *
 *  Copyright (c) 2003-2005, Jouni Malinen <jkmaline@cc.hut.fi>
 *  Copyright (C) 2001-2008, Iwncomm Ltd.
 */
#ifndef WAPI_ASUE_I_H_
#define WAPI_ASUE_I_H_

#define MAX_WAPI_SSID_LEN 64
#define BK_LEN 16

/* struct */
typedef enum
{
	/* No WAPI */
	AUTH_TYPE_NONE_WAPI = 0,
	/* Certificate */
	AUTH_TYPE_WAPI_CERT,
	/* Pre-PSK */
	AUTH_TYPE_WAPI_PSK,
}AUTH_TYPE;


typedef enum
{
	/* ascii */
	KEY_TYPE_ASCII = 0,
	/* HEX */
	KEY_TYPE_HEX,
}KEY_TYPE;


enum wapi_event_type {
	WAPI_EVENT_ASSOC = 0,
	WAPI_EVENT_DISASSOC,
};


typedef struct
{
	unsigned char v[6];
	unsigned char resv[2];
}MAC_ADDRESS;


typedef struct {
	/* Authentication type */
	AUTH_TYPE authType;
	/* WAPI_CERT */
	/* WAPI cert select mode */
	/* 0: automatic select; 1: user select one from the certificate list */
	unsigned int wapi_cert_sel_mode;
	/* User select cert alias */
	char wapi_cert_sel[256];
	/* WAPI_PSK Key type */
	KEY_TYPE kt;
	/* Key length */
	unsigned int kl;
	/* Key value */
	unsigned char kv[128];
} CNTAP_PARA;


struct wapi_config{
	u8 *ssid;
	size_t ssid_len;

	u8 *psk;
	size_t psk_len;
	u8 psk_bk[BK_LEN];
	int psk_set;

	int wapi_policy;

	unsigned int pairwise_cipher;
	unsigned int group_cipher;

	int key_mgmt;

	int proto;
	int auth_alg;
	char *cert_name;
	int disabled;
};

void wapi_config_free(struct wapi_config *config);

/* libwapi interface functions */
int wapi_asue_init();

int wapi_asue_deinit();

int wapi_asue_update_iface(void* iface);

int wapi_asue_set_config(const CNTAP_PARA* pPar);

void wapi_asue_event(enum wapi_event_type action,
		     const u8* pBSSID, const u8* pLocalMAC,
		     unsigned char *assoc_ie, unsigned char assoc_ie_len);

void wapi_asue_rx_wai(void *ctx, const u8 *src_addr, const u8 *buf, size_t len);

#endif /* WAPI_ASUE_I_H_ */
