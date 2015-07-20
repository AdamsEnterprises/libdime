#include <signet/keys.h>

static int             keys_check_length(const unsigned char *in, size_t in_len);
static EC_KEY *        keys_fetch_enc_key(const char *filename);
static ED25519_KEY *   keys_fetch_sign_key(const char *filename);
/* keys files not currently encrypted TODO */
/* private key serialization currently occurs into DER encoded format which is long, therefore we have 2 bytes for private key length TODO*/
static int             keys_file_create(keys_type_t type, ED25519_KEY *sign_key, EC_KEY *enc_key, const char *filename);
static unsigned char * keys_file_serialize(const char *filename, size_t *len);
static EC_KEY *        keys_serial_get_enc_key(const unsigned char *bin_keys, size_t len);
static ED25519_KEY *   keys_serial_get_sign_key(const unsigned char *bin_keys, size_t len);
static keys_type_t     keys_type_get(const unsigned char *bin_keys, size_t len);
/* not implemented yet TODO*/
/*
static int             keys_file_add_sok(ED25519_KEY *sok, const char *filename);
*/


/* PRIVATE FUNCTIONS */


/**
 * @brief	Checks the size of the keys buffer for consistency.
 * @param	in	Keys buffer.
 * @param	in_len	Keys buffer size.
 * @return	0 if the length checks pass, -1 if they do not.
*/
static int keys_check_length(const unsigned char *in, size_t in_len) {

	uint32_t signet_length;

	if (!in || (in_len < SIGNET_HEADER_SIZE)) {
		RET_ERROR_INT(ERR_BAD_PARAM, NULL);
	}

	signet_length = _int_no_get_3b((void *)(in + 2));

	if ((in_len - SIGNET_HEADER_SIZE) != signet_length) {
		RET_ERROR_INT(ERR_UNSPEC, "length does not match input size");
	}

	return 0;
}

/**
 * @brief	Retrieves the keys type (user or organizational) from the keys binary.
 * @param	bin_keys	Pointer to the keys buffer.
 * @param	len		Length of the keys buffer.
 * @return	Keys type on success, KEYS_TYPE_ERROR on error.
*/
static keys_type_t keys_type_get(const unsigned char *bin_keys, size_t len) {

	dime_number_t number;

	if(!bin_keys) {
		RET_ERROR_CUST(KEYS_TYPE_ERROR, ERR_BAD_PARAM, NULL);
	} else if(keys_check_length(bin_keys, len) < 0) {
		RET_ERROR_CUST(KEYS_TYPE_ERROR, ERR_BAD_PARAM, NULL);
	}

	number = (dime_number_t)_int_no_get_2b((void *)bin_keys);

	if (number == DIME_ORG_KEYS) {
		return KEYS_TYPE_ORG;
	} else if (number == DIME_USER_KEYS) {
		return KEYS_TYPE_USER;
	}

	RET_ERROR_CUST(KEYS_TYPE_ERROR, ERR_UNSPEC, "DIME number is not keys file type");
}

/**
 * @brief	Retrieves the encryption key from the keys binary.
 * @param	bin_keys        Pointer to the keys buffer.
 * @param	len		Length of the keys buffer.
 * @return	Pointer to elliptic curve key, NULL if an error occurred.
 * @free_using{free_ec_key}
*/
static EC_KEY *keys_serial_get_enc_key(const unsigned char *bin_keys, size_t len) {

        /* unsigned char sign_fid, enc_fid; sign_fid is unused causing errors on 
           compilation */
        unsigned char enc_fid;
	size_t at = 0, privkeylen;
	EC_KEY *enc_key = NULL;

	if(!bin_keys) {
		RET_ERROR_PTR(ERR_BAD_PARAM, NULL);
	} else if(keys_check_length(bin_keys, len) < 0) {
		RET_ERROR_PTR(ERR_BAD_PARAM, NULL);
	}

	switch(keys_type_get(bin_keys, len)) {

	case KEYS_TYPE_ORG:
		/* sign_fid = KEYS_ORG_PRIVATE_POK; */
		enc_fid = KEYS_ORG_PRIVATE_ENC;
		break;
	case KEYS_TYPE_USER:
		/* sign_fid = KEYS_USER_PRIVATE_SIGN; */
		enc_fid = KEYS_USER_PRIVATE_ENC;
		break;
	default:
		RET_ERROR_PTR(ERR_UNSPEC, "invalid keys type");
		break;

	}

	at = KEYS_HEADER_SIZE;

	while(bin_keys[at++] != enc_fid) {
		at += bin_keys[at] + 1;

		if(len <= at) {
			RET_ERROR_PTR(ERR_UNSPEC, "no private encryption key in keys file");
		}
	}

	privkeylen = _int_no_get_2b(bin_keys+at);
	at += 2;

	if(at + privkeylen > len) {
		RET_ERROR_PTR(ERR_UNSPEC, "invalid encryption key size");
	}

	if(!(enc_key = _deserialize_ec_privkey(bin_keys + at, privkeylen, 0))) {
		RET_ERROR_PTR(ERR_UNSPEC, "could not deserialize private EC encryption key");
	}

	return enc_key;
}

/**
 * @brief Retrieves the signing key from the keys binary.
 * @param	bin_keys	Pointer to the keys buffer.
 * @param	len		Length of the keys buffer.
 * @return	Pointer to ed25519 signing key, NULL if an error occurred.
 * @free_using{free_ed25519_key}
*/
static ED25519_KEY *keys_serial_get_sign_key(const unsigned char *bin_keys, size_t len) {

	unsigned char sign_fid;
	unsigned int at = 0;
	ED25519_KEY *sign_key;

	if(!bin_keys) {
		RET_ERROR_PTR(ERR_BAD_PARAM, NULL);
	} else if(keys_check_length(bin_keys, len) < 0) {
		RET_ERROR_PTR(ERR_BAD_PARAM, NULL);
	} else if(len < KEYS_HEADER_SIZE + 2 + ED25519_KEY_SIZE) {
		RET_ERROR_PTR(ERR_BAD_PARAM, "keys buffer too small for signing key");
	}

	switch(keys_type_get(bin_keys, len)) {

	case KEYS_TYPE_ORG:
		sign_fid = KEYS_ORG_PRIVATE_POK;
		break;
	case KEYS_TYPE_USER:
		sign_fid = KEYS_USER_PRIVATE_SIGN;
		break;
	default:
		RET_ERROR_PTR(ERR_UNSPEC, "invalid keys type");
		break;

	}

	at = KEYS_HEADER_SIZE;

	if(bin_keys[at++] != sign_fid) {
		RET_ERROR_PTR(ERR_UNSPEC, "no signing key was found");
	}

	if(bin_keys[at++] != ED25519_KEY_SIZE) {
		RET_ERROR_PTR(ERR_UNSPEC, "invalid size of signing key");
	}

	if(!(sign_key = _deserialize_ed25519_privkey(bin_keys + at))) {
		RET_ERROR_PTR(ERR_UNSPEC, "could not deserialize ed25119 signing key");
	}

	return sign_key;
}

/**
 * @brief	Retrieves the keys binary from the keys file.
 * @param	filename	Null terminated string containing specified filename.
 * @param	len		Pointer to the length of the output.
 * @return	Pointer to the keys binary string, this memory needs to be wipe before being freed. NULL on error.
 * @free_using{free}
*/
static unsigned char *keys_file_serialize(const char *filename, size_t *len) {

	char *b64_keys = NULL;
	unsigned char *serial_keys = NULL;

	if(!filename || !len) {
		RET_ERROR_PTR(ERR_BAD_PARAM, NULL);
	}

	if(!(b64_keys = _read_pem_data(filename, SIGNET_PRIVATE_KEYCHAIN, 1))) {
		RET_ERROR_PTR(ERR_UNSPEC, "could not retrieve keys from PEM file");
	}

	if(!(serial_keys = _b64decode(b64_keys, strlen(b64_keys), len))) {
		free(b64_keys);
		RET_ERROR_PTR(ERR_UNSPEC, "could not base64 decode the keys");
	}

	free(b64_keys);

	return serial_keys;
}

/**
 * @brief	Creates a keys file with specified signing and encryption keys.
 * @param	type	        Type of keys file, whether the keys correspond to a user or organizational signet.
 * @param	sign_key        Pointer to the specified ed25519 key, the private portion of which will be stored in the keys file as the signing key.
 * @param	enc_key		Pointer to the specified elliptic curve key, the private portion of which will be stored in the keys file as the encryption key.
 * @param	filename	Pointer to the NULL terminated string containing the filename for the keys file.
 * @return	0 on success, -1 on failure.
*/
static int keys_file_create(keys_type_t type, ED25519_KEY *sign_key, EC_KEY *enc_key, const char *filename) {

	char *b64_keys = NULL;
	int res;
	size_t serial_size = 0, enc_size = 0, at = 0;;
	unsigned char *serial_keys = NULL, *serial_enc = NULL, serial_sign[ED25519_KEY_SIZE], sign_fid, enc_fid;
	dime_number_t number;

	if(!sign_key || !enc_key || !filename) {
		RET_ERROR_INT(ERR_BAD_PARAM, NULL);
	}

	switch(type) {

	case KEYS_TYPE_ORG:
		number = DIME_ORG_KEYS;
		sign_fid = KEYS_ORG_PRIVATE_POK;
		enc_fid = KEYS_ORG_PRIVATE_ENC;
		break;
	case KEYS_TYPE_USER:
		number = DIME_USER_KEYS;
		sign_fid = KEYS_USER_PRIVATE_SIGN;
		enc_fid = KEYS_USER_PRIVATE_ENC;
		break;
	default:
		RET_ERROR_INT(ERR_BAD_PARAM, NULL);
		break;

	}

	memcpy(serial_sign, sign_key->private_key, ED25519_KEY_SIZE);

	if(!(serial_enc = _serialize_ec_privkey(enc_key, &enc_size))) {
		_secure_wipe(serial_sign, ED25519_KEY_SIZE);
		RET_ERROR_INT(ERR_UNSPEC, "could not serialize private key");
	}

	serial_size = KEYS_HEADER_SIZE + 1 + 1 + ED25519_KEY_SIZE + 1 + 2 + enc_size;

	if(!(serial_keys = malloc(serial_size))) {
		PUSH_ERROR_SYSCALL("malloc");
		_secure_wipe(serial_sign, ED25519_KEY_SIZE);
		_secure_wipe(serial_enc, enc_size);
		free(serial_enc);
		RET_ERROR_INT(ERR_NOMEM, NULL);
	}

	memset(serial_keys, 0, serial_size);
	_int_no_put_2b(serial_keys, (uint16_t)number);
	_int_no_put_3b(serial_keys + 2, (uint32_t)(serial_size - KEYS_HEADER_SIZE));
	at = KEYS_HEADER_SIZE;
	serial_keys[at++] = sign_fid;
	serial_keys[at++] = ED25519_KEY_SIZE;
	memcpy(serial_keys + at, serial_sign, ED25519_KEY_SIZE);
	at += ED25519_KEY_SIZE;
	_secure_wipe(serial_sign, ED25519_KEY_SIZE);
	serial_keys[at++] = enc_fid;
	_int_no_put_2b(serial_keys + at, (uint16_t)enc_size);
	at += 2;
	memcpy(serial_keys + at, serial_enc, enc_size);
	_secure_wipe(serial_enc, enc_size);
	free(serial_enc);

	b64_keys = _b64encode(serial_keys, serial_size);
	_secure_wipe(serial_keys, serial_size);
	free(serial_keys);

	if (!b64_keys) {
		RET_ERROR_INT(ERR_UNSPEC, "could not base64 encode the keys");
	}

	res = _write_pem_data(b64_keys, SIGNET_PRIVATE_KEYCHAIN, filename);
	_secure_wipe(b64_keys, strlen(b64_keys));
	free(b64_keys);

	if(res < 0) {
		RET_ERROR_INT(ERR_UNSPEC, "could not store keys in PEM file.");
	}

	return 0;
}

/**
 * @brief	Retrieves the signing key from the keys file.
 * @param	filename	Null terminated filename string.
 * @return	Pointer to the ed25519 signing key.
 * @free_using{free_ed25519_key}
*/
static ED25519_KEY *keys_fetch_sign_key(const char *filename) {

	size_t keys_len;
	unsigned char *keys_bin;
	ED25519_KEY *key;

	if(!filename) {
		RET_ERROR_PTR(ERR_BAD_PARAM, NULL);
	} else if(!strlen(filename)) {
		RET_ERROR_PTR(ERR_BAD_PARAM, NULL);
	}

	if(!(keys_bin = keys_file_serialize(filename, &keys_len))) {
		RET_ERROR_PTR(ERR_UNSPEC, "could not retrieve keys binary string");
	}

	key = keys_serial_get_sign_key(keys_bin, keys_len);
	_secure_wipe(keys_bin, keys_len);
	free(keys_bin);

	if (!key) {
		RET_ERROR_PTR(ERR_UNSPEC, "could not retrieve ed25519 signing key");
	}

	return key;
}

/**
 * @brief	Retrieves the encryption key from the keys file.
 * @param	filename	Null terminated filename string.
 * @return	Pointer to the elliptic curve encryption key.
 * @free_using{free_ec_key}
*/
static EC_KEY *keys_fetch_enc_key(const char *filename) {

	size_t keys_len;
	unsigned char *keys_bin;
	EC_KEY *key;

	if(!filename) {
		RET_ERROR_PTR(ERR_BAD_PARAM, NULL);
	} else if(!strlen(filename)) {
		RET_ERROR_PTR(ERR_BAD_PARAM, NULL);
	}

	if(!(keys_bin = keys_file_serialize(filename, &keys_len))) {
		RET_ERROR_PTR(ERR_UNSPEC, "could not retrieve keys binary string");
	}

	key = keys_serial_get_enc_key(keys_bin, keys_len);
	_secure_wipe(keys_bin, keys_len);
	free(keys_bin);

	if (!key) {
		RET_ERROR_PTR_FMT(ERR_UNSPEC, "could not retrieve ed25519 signing key from %s", filename);
	}

	return key;
}


/* PUBLIC FUNCTIONS */

int dime_keys_file_create(keys_type_t type, ED25519_KEY *sign_key, EC_KEY *enc_key, const char *filename) {
	PUBLIC_FUNCTION_IMPLEMENT(keys_file_create, type, sign_key, enc_key, filename);
}

/* not implemented yet TODO*/
/*
int dime_keys_file_add_sok(ED25519_KEY *sok, const char *filename) {
	PUBLIC_FUNCTION_IMPLEMENT(keys_file_add_sok, sok, filename);
}
*/

ED25519_KEY *dime_keys_fetch_sign_key(const char *filename) {
	PUBLIC_FUNCTION_IMPLEMENT(keys_fetch_sign_key, filename);
}

EC_KEY *dime_keys_fetch_enc_key(const char *filename) {
	PUBLIC_FUNCTION_IMPLEMENT(keys_fetch_enc_key, filename);
}
