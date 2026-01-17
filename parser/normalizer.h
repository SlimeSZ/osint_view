#ifndef NORMALIZER_H
#define NORMALIZER_H

#include "tokenizer.h"



/* lower-casing */
static inline char *token_to_lower(const Token *t);
static inline char *tokenlist_to_lower(const TokenList *tl);
static inline char *str_to_lower(const char *str);

/* Possessive/Punctuational Stripping */
void strip_edges(const char **start, const char **end);
void strip_possessives(const char **start, const char **end);
void normalize_abbreviation(const char *start, const char *end, 
					char *out_buf, size_t *out_len);

/* TBD */
void *normalize_token();

#endif
