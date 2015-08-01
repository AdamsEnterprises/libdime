/**
 * @file
 * @brief	Function declarations and types used by the structured format parsers.
 */

#ifndef MAGMA_CORE_PARSERS_FORMATS_H
#define MAGMA_CORE_PARSERS_FORMATS_H

typedef struct {
	MAGMA_INDEX options;
	struct {
		char comment, value, line;
	} tokens;
	inx_t *pairs;
} nvp_t;

nvp_t *nvp_alloc(void);
void   nvp_free(nvp_t *nvp);
void   nvp_init(nvp_t *nvp);
int    nvp_parse(nvp_t *nvp, stringer_t *data);

#endif /* FORMATS_H_ */
