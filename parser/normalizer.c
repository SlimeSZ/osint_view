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

void strip_edges(const char **start_in, const char **end_in) {
	if (!start_in || !end_in || *start_in > *end_in)
		return;
	const char *s = *start_in;
	const char *e = *end_in;

	while (s < e) {
		unsigned char c = (unsigned char)*s;
		if (!ispunct(c) || c == '\'') 
			break;
		s++;
	}

	while (e > s) {
		unsigned char c = (unsigned char)*(e - 1);
		if (!ispunct(c))
			break;
		e--;
	}

	*start_in = s;
	*end_in = e;
}

static inline void strip_possessive(const char **start_in, const char **end_in) {
	if (!start_in || !end_in) return;

	const char *s = *start_in;
	const char *e = *end_in;
	size_t len = e - s;

	if (len >= 2 && e[-2] == '\'' && (e[-1] == 's' || e[-1] == 'S')) {
		// strip 's or 'S 
	        e -= 2;
	} else if (len >= 1 && e[-1] == '\'') {
		// strip trailing apostrophe only 
	        e -= 1;
	}
	*start_in = s;
	*end_in   = e;
}

static inline void normalize_abbreviation(const char *start, const char *end,
                                          char *out_buf, size_t *out_len) {
	if (!start || !end || !out_buf || !out_len) return;
	size_t len = end - start;
	size_t j = 0;

    	// must be at least 3 chars e.g. 'X.Y' to be abbreviation
    	if (len < 3) {
        // copy as is
        for (size_t i = 0; i < len; i++) out_buf[i] = start[i];
        	*out_len = len;
        	return;
    	}

    	// pattern check: Letter-Period-Letter-Period...
    	int is_abbrev = 1;
    	for (size_t i = 0; i < len; i++) {
        	if (i % 2 == 0) {          // even indices: letters
            	if (!isalpha((unsigned char)start[i])) { is_abbrev = 0; break; }
        	} else {                   // odd indices: period
            		if (start[i] != '.') { is_abbrev = 0; break; }
        	}
    	}

    	if (!is_abbrev) {
        	// not abbreviation: copy as-is
        	for (size_t i = 0; i < len; i++) out_buf[i] = start[i];
        	*out_len = len;
        	return;
    	}

    	// strip periods
    	for (size_t i = 0; i < len; i++) {
        	if (start[i] != '.') {
            	out_buf[j++] = start[i];
        	}
    	}

    	*out_len = j;
}
