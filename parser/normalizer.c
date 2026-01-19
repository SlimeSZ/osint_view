#include "normalizer.h"
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "tokenizer.h"

static inline char *token_to_lower(const Token *t) {
	size_t n = t->end - t->start;
	char *out = malloc(n + 1);
	if (!out) { perror("malloc to_lower()"); return NULL; }

	for (size_t i = 0; i < n; i++) {
		out[i] = tolower((unsigned char)t->start[i]);
	}
	out[n] = '\0';
	return out;
}

static inline char *tokenlist_to_lower(const TokenList *tl) {
	if (!tl || tl->size == 0) return NULL;
	
	size_t n = tl->tokens[tl->size - 1].end - tl->tokens[0].start;
	char *out = malloc(n + 1);
	if (!out) { perror("malloc tokenlist_to_lower()"); return NULL; }

	const char *p = tl->tokens[0].start;
	char *dst = out;

	for (size_t i = 0; i < tl->size; i++) {
		Token t = tl->tokens[i];
		for (size_t j = 0; j < t.len; j++) {
			*dst++ = tolower(t.start[j]);
		}
	}
	*dst = '\0';
	return out;
}

static inline char *str_to_lower(const char *str) {
	size_t n = strlen(str);
	char *out = malloc(n + 1);
	if (!out) { perror("malloc str_to_lower()"); return NULL; }
	
	for (size_t i = 0; i < n; i++) {
		out[i] = tolower(str[i]);
	}
	out[n] = '\0';
	return out;
}

void normalize_(const TokenList *tokens, char **out_tokens) {
	if (!tokens || tokens->size == 0 || !out_tokens)
		return;

	size_t outc = 0;
	for (size_t i = 0; i < tokens->size; i++) {
		Token *t = &tokens->tokens[i];
		
		// printf("(%zu)[%.*s] -- \n", i, (int)(t->end - t->start), t->start);
		if (t->type == TOKEN_WHITESPACE || t->type == TOKEN_PUNCT) {
			// printf("token '%.*s' is type: %d\n", (int)(t->end-t->start), t->start, t->type);
			continue;
		}
		
		/* one token is in range [s, e] */
		const char *s = t->start;
		const char *e = t->end;

		while (s < e && ispunct((unsigned char)*s))
			s++;
		// printf("end: %c\n", (unsigned char)*(e - 1));
		size_t n = e-s; 

		// temp buffer for normalized output
		char nbuf[256];
		size_t bidx = 0;

		bool is_abbrev = (n >= 3);
		
		/* All normalziations involving iterating over a token's chars are performed here */
		// printf("--- Token Word [%zu] ---\n", i);
		for (size_t j = 0; j < n; j++) {
			char c = s[j];
			// printf("char: [%zu] = %c\n", i, c);
			
			/* Abbreviation normalziation */
			if (is_abbrev) {
				if (j % 2 == 0) { // if even it's a letter
					if (!isalpha((unsigned char)c)) {
						is_abbrev = false;
					}
				} else { // if odd its a period
					if (c != '.') {
						is_abbrev = false;
					}
				}
			}	
			// copy to buffer, skipping dots if abrev. and add lowercase normalization
			if (c != '.' || !is_abbrev) {
				// printf("copying char: '%c' to buf[%zu]\n", c, bidx);
				nbuf[bidx++] = tolower((unsigned char)c);
			} 
			// debug
			// else {
				// printf("skipping char: '%c' (dot in abbreviation)\n", c);
			// }


		}
		if (bidx >= 2 && nbuf[bidx-2] == '\'' && (nbuf[bidx-1] == 's' || nbuf[bidx-1] == 'S')) {
			bidx -= 2;  // posessive strip 
		} else if (bidx >= 1 && nbuf[bidx-1] == '\'') { 
			bidx -= 1;  // trailing apostrophe strip
		}

		nbuf[bidx] = '\0';
		out_tokens[outc++] = strdup(nbuf);
		// printf("Token success: %s\n", nbuf);
	}
	out_tokens[outc] = NULL;
}

void free_normalized(char **normalized) {
	if (!normalized) return;
	for (size_t i = 0; normalized[i] != NULL; i++) {
		free(normalized[i]);
	}
}

/* Usage */
/* gcc -o ./builds/test tokenizer.c normalizer.c */
int main(void) {
    const char *t = "don't John's 'quoted' self-destruct"; 
    TokenList *list = NULL;
    tokenize(t, &list);

    char *buf[256];
    normalize_(list, buf);

    for (size_t i = 0; buf[i] != NULL; i++) {
        printf("%s ", buf[i]);
    }

    free_normalized(buf);
    free_tokens(list);
}
