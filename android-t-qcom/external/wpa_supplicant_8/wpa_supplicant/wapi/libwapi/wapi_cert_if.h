/*
 * Copyright (C) 2016-2020 China IWNCOMM Co., Ltd. All rights reserved.
 *
 * Contains function declarations for wapi certificate manage interface
 *
 * Authors:
 * yucun tian
 *
 * History:
 * yucun tian  06/06/2016 v1.0 write the code.
 *
 * Related documents:
 * -GB/T 15629.11-2003/XG1-2006
 * -ISO/IEC 8802.11:1999,MOD
 *
*/

#ifndef _WAPI_CERT_IF_H_
#define _WAPI_CERT_IF_H_

#include "cert_parse.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* Key prefix for WAPI */
//#define WAPI "WAPI_"
/* Key prefix for WAPI CA certificates */
#define WAPI_CA_CERTIFICATE "WAPI_CA_"
/* Key prefix for WAPI user certificates */
#define WAPI_USER_CERTIFICATE "WAPI_USER_"
/* Key prefix for WAPI user private keys */
#define WAPI_USER_PRIVATE_KEY "WAPI_KEY_"

/* WAPI certificate state config file name */
#define CERT_STATE_CONFIG_FILE_NAME "/data/vendor/wifi/wapi_cert_state.conf"

typedef struct __cert_id
{
	u16 cert_flag;
	u16 length;
	u8 data[2048];
}cert_id;

typedef struct _wai_fixdata_id
{
	u16	id_flag;
	u16	id_len;
	u8 	id_data[1000];
}wai_fixdata_id;

typedef struct _comm_data
{
	u16 length;
	u16 pad_value;
	u8 data[2048];
}comm_data, *pcomm_data,
 tkey, *ptkey, tsign;

/* Description wapi certificate list and status */
typedef struct _wapi_cert_data
{
	/* cert flag */
	int cert_flag;
	/* cert count */
	int cert_count;
	/* user cert sel mode flag */
	int wapi_cert_sel_mode;
	/* user cert sel name */
	char wapi_cert_sel[256];
	struct _CERT_STATE *cert_state_list;
	struct _wapi_x509_cert_asue_data *cert_asue_data;
	struct _wapi_x509_cert_asue_data *asue_cert_cur;
}wapi_cert_data;

/* Trusted as certificate struct */
typedef struct _wapi_x509_cert_as_data
{
	/* Public key */
	tkey *asu_pubkey;
	/* Identity */
	wai_fixdata_id *as_id;
	/* Certificate content */
	cert_id *as_cert_st;
	/* Next certificate */
	struct _wapi_x509_cert_as_data *next;
}wapi_x509_cert_as_data;

/* ASUE certificate struct */
typedef struct _wapi_x509_cert_asue_data
{
	/* Certificate alias */
	char cert_rename[128];
	/* Certificate state */
	char cert_state[8];
	/* User identity */
	wai_fixdata_id *user_id;
	/* Issuer identity*/
	wai_fixdata_id *asu_id;
	/* Issuer certificate content */
	cert_id *asu_cert_st;
	/* Issuer public key */
	tkey *asu_pubkey;
	/* User certificate content */
	cert_id *user_cert_st;
	/* Private key */
	tkey *private_key;
	/* Trusted as certificate */
	wapi_x509_cert_as_data *as;
	/* Next item*/
	struct _wapi_x509_cert_asue_data *next;
}wapi_x509_cert_asue_data;

/* WAPI certificate state */
typedef struct _CERT_STATE
{
	char *alias;
	int alias_len;
	char *state;
	int state_len;
	struct _CERT_STATE *next;
}CERT_STATE;

wapi_x509_cert_asue_data *alloc_asue_cert_data();

void free_asue_cert_data(wapi_x509_cert_asue_data *asue_cert_data);

void free_aliases(char** aliases, int count);

/*
 * get_asue_cert_list - get asue certificate data list
 * @aliases: it points to a list of cert's aliases
 * @p_asue_cert_list: it is a pointer to the asue certificate list
 * that will be malloc() and the caller is responsible for calling free()
 * on the buffer
 *
 * Returns: if success, return list count; else, return -1
*/

int get_asue_cert_list(wapi_x509_cert_asue_data **p_asue_cert_list);

/*
 * get_asue_cert_use_alias - get asue certificate data by certificate alias
 * @alias: certificate alias
 *
 * Returns: if success, return certificate data; else, return NULL
*/

wapi_x509_cert_asue_data* get_asue_cert_use_alias(char *alias);

/*
 * free_asue_cert_list - Free wapi cert list memory
 * @asue_cert_list: Pointer to the asue certificate list
 *
 * Returns: void
*/

void free_asue_cert_list(wapi_x509_cert_asue_data* asue_cert_list);

/*
 * add_cert_items - Add certificate items
 * @asue_cert_data: asue certificate
 * @user_cert: user certificate
 * @ca_cert: ca certificate
 * @pri_key: user private key
 * @alias: certificate alias
 * @cert_state: certificate state
 *
 * Returns: if success, return 0; else, return -1
*/

int add_cert_items(wapi_x509_cert_asue_data* asue_cert_data,
					u8 *user_cert, int user_cert_len,
					u8 *ca_cert, int ca_cert_len,
					u8 *pri_key, int pri_key_len,
					char *alias, char *cert_state);

/*
 * get_cert_identity - Get certificate identity
 * @cert_st: certificate content
 * @fixdata_id: out param, certificate identity
 *
 * Returns: if success, return 0; else, return -1
*/

int get_cert_identity(cert_id *cert_st,
					wai_fixdata_id *fixdata_id,
					unsigned short index);


/* Get certificate by index */
wapi_x509_cert_asue_data* get_asue_cert_use_index(int index,
					wapi_x509_cert_asue_data *asue_cert_list);

/*
 * get_cert_state_list - Get the certificate state list from config file
 * @p_cert_state_list: Pointer to the certificate state list
 * that will be malloc() and the caller is responsible for calling free()
 * on the buffer
 *
 * Returns: if success, return list count; else, return -1
*/

int get_cert_state_list(CERT_STATE **p_cert_state_list);

/*
 * save_cert_state_list - save the certificate state list to config file
 * @cert_state_list: certificate state list
 *
 * Returns: if success, return 0; else, return -1
*/

int save_cert_state_list(CERT_STATE *cert_state_list);

/*
 * free_cert_state_list - free the certificate state list memory
 * @cert_state_list: certificate state list
 *
 * Returns: void
*/

void free_cert_state_list(CERT_STATE *cert_state_list);

/*
 * add_cert_state - Add certificate state item to the certificate state list
 * @p_cert_state_list: Pointer to the certificate state list
 * @alias: Certificate alias
 * @state: Certificate state
 *
 * Returns: if success, return 0; else, return -1
*/

int add_cert_state(CERT_STATE **p_cert_state_list, char *alias, char *state);

/*
 * match_cert_state - Match certificate list and certificate state list,
 * set state to the certificate list.
 * @asue_cert_list: certificate list
 * @p_cert_state_list: Pointer to the certificate state list
 *
 * Returns: if success, return 0; else, return -1
*/

int match_cert_state(wapi_x509_cert_asue_data *asue_cert_list,
		     CERT_STATE **p_cert_state_list);

/* Initialize wapi certificate list and status */
int wapi_certstore_init(wapi_cert_data *cert_info);

/* Deinit wapi certificate list */
void wapi_certstore_deinit(wapi_cert_data *cert_info);

/* Get a matching certificate from the certificate list */
int wapi_get_cert_by_asu_id(wapi_cert_data *cert_info, wai_fixdata_id *ae_asu_id);

#ifdef  __cplusplus
}
#endif

#endif /* _WAPI_CERT_IF_H_  */

