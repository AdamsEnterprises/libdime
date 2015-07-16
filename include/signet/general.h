#ifndef SIGNET_GENERAL_H
#define SIGNET_GENERAL_H

#define SIGNET_VER_NO           0x1
#define SIGNET_HEADER_SIZE      5
#define SIGNET_MAX_SIZE         16777220
#define SIGNET_PRIVATE_KEYCHAIN "SIGNET PRIVATE KEYCHAIN"
#define SIGNET_PEM_TAG          "SIGNET"
#define KEYS_HEADER_SIZE        5
#define FIELD_NAME_MAX_SIZE     255
#define UNSIGNED_MAX_1_BYTE     255
#define UNSIGNED_MAX_2_BYTE     65535
#define UNSIGNED_MAX_3_BYTE     16777215
#define SIGNET_FID_MAX          255
#define KEYS_FID_MAX            3
#define DIME_NUMBER_SIZE        2

#include <common/error.h>
#include <common/misc.h>
#include <common/dcrypto.h>

typedef enum {
	SIGNET_TYPE_ERROR,
	SIGNET_TYPE_ORG = 1,
	SIGNET_TYPE_USER,
	SIGNET_TYPE_SSR
} signet_type_t;


typedef struct {
	signet_type_t type;
	uint32_t fields[256];           /**< Each index corresponds to a different field type identifier. The value of fields[index] is the byte directly after the first occurence of the corresponding field type identifier. */
	                                /**< If fields[index] is 0 it means that the corresponding field type identifier occurred 0 times.*/
	uint32_t size;                  /**< Combined length of all the fields */
	unsigned char *data;
} signet_t;

typedef enum dime_number_t {
	DIME_ORG_SIGNET = 1776,         /**< File contains an organizational signet */
	DIME_USER_SIGNET = 1789,        /**< File contains a user signet */
	DIME_SSR = 1216,                /**< File contains an ssr*/
	DIME_ORG_KEYS = 1952,           /**< File contains organizational keys*/
	DIME_USER_KEYS = 2013,          /**< File contains user keys*/
	DIME_MSG_TRACING = 1837,
	DIME_ENCRYPTED_MSG = 1847
} dime_number_t;

typedef enum {
	KEYS_TYPE_ERROR = 0,
	KEYS_TYPE_ORG,
	KEYS_TYPE_USER
} keys_type_t;

typedef enum {
	SIGNET_ORG_POK = 1,             /**< The ed25519 public signing key of the signet holder */
	SIGNET_ORG_SOK,                 /**< Secondary Organization Signing keys */
	SIGNET_ORG_ENC_KEY,             /**< The ECC public encryption key of the signet holder */
	SIGNET_ORG_CRYPTO_SIG,          /**< Org signature of all previous fields */
	SIGNET_ORG_NAME = 16,
	SIGNET_ORG_ADDRESS,
	SIGNET_ORG_PROVINCE,
	SIGNET_ORG_COUNTRY,
	SIGNET_ORG_POSTAL,
	SIGNET_ORG_PHONE,
	SIGNET_ORG_LANGUAGE,
	SIGNET_ORG_CURRENCY,
	SIGNET_ORG_CRYPTOCURRENCY,
	SIGNET_ORG_MOTTO,
	SIGNET_ORG_EXTENSIONS,
	SIGNET_ORG_MSG_SIZE_LIM,
	SIGNET_ORG_WEBSITE = 160,
	SIGNET_ORG_ABUSE = 200,
	SIGNET_ORG_ADMIN,
	SIGNET_ORG_SUPPORT,
	SIGNET_ORG_WEB_HOST,
	SIGNET_ORG_WEB_LOCATION,
	SIGNET_ORG_WEB_CERT,
	SIGNET_ORG_MAIL_HOST,
	SIGNET_ORG_MAIL_CERT,
	SIGNET_ORG_ONION_ACCESS_HOST,
	SIGNET_ORG_ONION_ACCESS_CERT,
	SIGNET_ORG_ONION_DELIVERY_HOST,
	SIGNET_ORG_ONION_DELIVERY_CERT,
	SIGNET_ORG_UNDEFINED = 251,     /**< Unicode undefined field*/
	SIGNET_ORG_PHOTO,               /**< Organizational photo*/
	SIGNET_ORG_FULL_SIG,            /**< ORG signature*/
	SIGNET_ORG_ID,                  /**< Org Signet ID */
	SIGNET_ORG_ID_SIG             /**< Org Signature following the ID field */
} SIGNET_ORG_FIELD_T;

typedef enum {
	SIGNET_USER_SIGN_KEY = 1,       /**< The ed25519 public signing key of the signet holder*/
	SIGNET_USER_ENC_KEY,            /**< The ECC public encryption key of the signet holder*/
	SIGNET_USER_ALT_KEY,            /**< Alternative encryption keys for the user */
	SIGNET_USER_COC_SIG,            /**< Chain of custody signature by user's previous signing key*/
	SIGNET_USER_SSR_SIG,            /**< User signature with user's signing key*/
	SIGNET_USER_CRYPTO_SIG,        /**< Initial signature by the organization's signing key*/
	SIGNET_USER_NAME = 16,
	SIGNET_USER_ADDRESS,
	SIGNET_USER_PROVINCE,
	SIGNET_USER_COUNTRY,
	SIGNET_USER_POSTAL,
	SIGNET_USER_PHONE,
	SIGNET_USER_LANGUAGE,
	SIGNET_USER_CURRENCY,
	SIGNET_USER_CRYPTOCURRENCY,
	SIGNET_USER_MOTTO,
	SIGNET_USER_EXTENSIONS,
	SIGNET_USER_MSG_SIZE_LIM,
	SIGNET_USER_CODECS = 93,
	SIGNET_USER_TITLE,
	SIGNET_USER_EMPLOYER,
	SIGNET_USER_GENDER,
	SIGNET_USER_ALMA_MATER,
	SIGNET_USER_SUPERVISOR,
	SIGNET_USER_POLITICAL_PARTY,
	SIGNET_USER_ALTERNATE_ADDRESS = 200,
	SIGNET_USER_RESUME,
	SIGNET_USER_ENDORSEMENTS,
	SIGNET_USER_UNDEFINED = 251,    /**< ASCII undefined field*/
	SIGNET_USER_PHOTO,              /**< User photo*/
	SIGNET_USER_FULL_SIG,           /**< Final Organizational Signature*/
	SIGNET_USER_ID,                 /**< User Signet ID */
	SIGNET_USER_ID_SIG            /**< Org Signature following the ID field */
} SIGNET_USER_FIELD_T;

typedef enum {
	SIGNET_SSR_SIGN_KEY = 1,        /**< The proposed ed25519 public signing key of the ssr creator*/
	SIGNET_SSR_ENC_KEY,             /**< The ed25519 ECC public encryption key of the ssr creator*/
	SIGNET_SSR_ALT_KEY,             /**< Alternative encryption keys for the ssr creator */
	SIGNET_SSR_COC_SIG,             /**< Chain of custody signature by user's previous signing key*/
	SIGNET_SSR_SSR_SIG,             /**< User signature with user's signing key*/
} SIGNET_SSR_FIELD_T;

typedef enum {
	KEYS_ORG_PRIVATE_POK = 1,
	KEYS_ORG_PRIVATE_SOK,
	KEYS_ORG_PRIVATE_ENC,
} KEYS_ORG_T;

typedef enum {
	KEYS_USER_PRIVATE_SIGN = 1,
	KEYS_USER_PRIVATE_ENC,
} KEYS_USER_T;

typedef enum {
	SIGNKEY_DEFAULT_FORMAT =    0x40,    /**< Currently the only legal format specifier for ED25519 signing keys*/
} signkey_format_t;

typedef enum {
	SIGNET_SOK_NONE =              0,   /**< Can not be used for signing anything */
	SIGNET_SOK_SIGNET =            1,   /**< Can be used for signing signets */
	SIGNET_SOK_MSG =               2,   /**< Can be used for signing messages */
	SIGNET_SOK_TLS =               4,   /**< Can be used for signing TLS certificates */
	SIGNET_SOK_SOFTWARE =          8    /**< Can be used for signing software */
} sok_permissions_t;

typedef enum {
	SS_UNKNOWN = 0,                 /**< Invalid signet, state unknown/currently unclassified */
	SS_MALFORMED,                   /**< Invalid signet, it either doesn't fit the field format or has multiple unique fields */
	SS_OVERFLOW,                    /**< Invalid signet due to it being too large. */
	SS_INCOMPLETE,                  /**< Invalid signet, it is missing fields required to fit one of the valid categories, likely unsigned */
	SS_BROKEN_COC,		        /**< Invalid signet due to chain of custody signature being invalid*/
	SS_INVALID,                     /**< Invalid signet, one or more signatures can not be verified */
	SS_SSR,                         /**< Valid unsigned SSR */
	SS_CRYPTO,                      /**< Valid cryptographic signet */
	SS_FULL,                        /**< Valid full signet */
	SS_ID,                          /**< Valid full signet with ID and organizational-identifiable-signature */
} signet_state_t;

typedef enum {                          /**< Currently barely used, meant to classify signet field data types*/
	B64,
	HEX,
	PNG,
	UNICODE
} field_data_t;

typedef struct {

/* field properties */

	unsigned int required;          /**< is this field required*/
	unsigned int unique;            /**< can there be multiple fields of this identifier */

	unsigned char bytes_name_size;  /**< Is this a defined field */
	unsigned char bytes_data_size;  /**< Number of bytes for this */
	uint32_t data_size;             /**< data_size = 0 indicates the size being variable */

	field_data_t data_type;         /**< Dump format for the field */

	const char *name;
	const char *description;        /**< field type description*/

} signet_field_key_t;

/** A signet field index structure for temporary convenience organization of field data */
typedef struct signet_field_t {

	const signet_t *signet;
	signet_field_key_t *key;
	unsigned char name_size;
	unsigned int data_size;

	unsigned int id_offset;
	unsigned int name_offset;
	unsigned int data_offset;

	struct signet_field_t *next;
} signet_field_t;

const char *dime_number_to_str(dime_number_t number);

extern signet_field_key_t signet_org_field_keys[256];
extern signet_field_key_t signet_user_field_keys[256];
extern signet_field_key_t signet_ssr_field_keys[256];

#endif
