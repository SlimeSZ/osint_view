#ifndef NORMALIZER_H
#define NORMALIZER_H

#include "tokenizer.h"



/* lower-casing utils */
static inline char *token_to_lower(const Token *t);
static inline char *tokenlist_to_lower(const TokenList *tl);
static inline char *str_to_lower(const char *str);

void normalize_(const TokenList *tokens, char **out_tokens);
void free_normalized(char **normalized);

#endif
