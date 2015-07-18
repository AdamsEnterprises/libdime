#ifndef DMSG_PARSER_H
#define DMSG_PARSER_H

#include "dmessage/dmime.h"

/**
 * @brief	Destroys a dmime_envelop_object_t structure.
 * @param	obj		Pointer to the object to be destroyed.
 */
void                        dime_prsr_envelope_destroy(dmime_envelope_object_t *obj);

/**
 * @brief	Formats the provided data into the format required by the origin and destination chunks.
 * @param	user_id		Stringer containing author or recipient email address.
 * @param	org_id		Stringer containing destination or origin domain.
 * @param	user_signet	Cstring containing author or recipient b64 encoded cryptographic signet.
 * @param	org_fp		Cstring containing b64 encoded fingerprint of destination or origin cryptographic signet.
 * @param	type		Type of envelope chunk (CHUNK_TYPE_ORIGIN or CHUNK_TYPE_DESTINATION).
 * @return	Stringer containing the envelope formatted data.
*/
stringer_t *                dime_prsr_envelope_format(stringer_t *user_id, stringer_t *org_id, const char *user_signet, const char *org_fp, dmime_chunk_type_t type);

/**
 * @brief	Parses a binary buffer from a dmime message into a dmime origin object.
 * @param	in		Binary origin array.
 * @param	insize		Size of input array.
 * @param	type		Type of the chunk.
 * @return	Pointer to a parsed dmime object or NULL on error.
 * @free_using{dime_prsr_envelope_destroy}
*/
dmime_envelope_object_t *   dime_prsr_envelope_parse(const unsigned char *in, size_t insize, dmime_chunk_type_t type);

/**
 * @brief	Allocates memory for an empty dmime_common_headers_t type.
 * @return	dmime_common_headers_t structure.
 * @free_using{dime_prsr_headers_destroy}
*/
dmime_common_headers_t *    dime_prsr_headers_create(void);

/**
 * @brief	Destroys a dmime_common_headers_t structure.
 * @param	obj		Headers to be destroyed.
*/
void                        dime_prsr_headers_destroy(dmime_common_headers_t *obj);

/**
 * @brief	Formats the dmime_common_headers_t into a single array for the common headers chunk.
 * @param	obj		The headers to be formatted.
 * @param	outsize	Stores the size of the output array.
 * @return	Returns the array of ASCII characters (not terminated by '\0') as pointer to unsigned char.
 * @free_using{free}
*/
unsigned char *             dime_prsr_headers_format(dmime_common_headers_t *obj, size_t *outsize);

/**
 * @brief	Parses the passed array of bytes into dmime_common_headers_t.
 * @param	in		Input buffer.
 * @param	insize	Input buffer size.
 * @return	A dmime_common_headers_t array of stringers containing parsed header info.
 * @free_using{dime_prsr_headers_destroy}
*/
dmime_common_headers_t *    dime_prsr_headers_parse(unsigned char *in, size_t insize);




#endif
