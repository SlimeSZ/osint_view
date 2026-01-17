#include "tokenizer.h" 
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define INITIAL_CAPACITY 128

static inline TokenList *token_list_init(void) {
	TokenList *list = malloc(sizeof(TokenList));
	if (__builtin_expect(!list, 0)) {
		return NULL;
	}
	list->tokens = malloc(INITIAL_CAPACITY * sizeof(Token));
	if (__builtin_expect(!list->tokens, 0)) {
		free(list);
		return NULL;
	}
	list->size = 0;
	list->capacity = INITIAL_CAPACITY;
	return list; 
}

static inline void add_token(
	TokenList *list, 
	const char *start, 
	const char *end,
	TokenType type 
	) {
	if (list->size >= list->capacity) {
		list->capacity *= 2;
		Token *new_token = realloc(list->tokens, list->capacity * sizeof(Token));
		if (!new_token) return;
		list->tokens = new_token;
	}
	size_t n = list->size;
	list->tokens[n].start = start;
	list->tokens[n].end = end;
	list->tokens[n].len = end - start;
	list->tokens[n].type = type;
	list->tokens[n].sentence_end = false;
	list->size++;
}

// helper for URL extraction
static bool is_url_char(char c) {
    return isalnum(c) || c == ':' || c == '/' || c == '.' || c == '-' ||
           c == '_' || c == '?' || c == '=' || c == '&';
}

void sentence_bounds(TokenList *list, const char *text_end) {
	if (list->size == 0) return;
	size_t curr = list->size - 1;
	Token *t = &list->tokens[curr];

	char c = t->start[0];
	if (c != '.' && c != '!' && c != '?' && c != '|') {
        	return; 
	}

	// decimal falsification (prev & next = num/digit)
	if (c == '.' && curr > 0) {
		Token *prev = &list->tokens[curr - 1];
		if (prev->type == TOKEN_NUMBER && t->end < text_end && isdigit(*t->end))
			return;
	}

	// ellipsis check (c + 1 == '.')
	if (c == '.' && t->end < text_end && *t->end == '.')
		return;

	// abbreviation (U.S.A) -- needs optimization
	if (c == '.' && curr > 0) {
		Token *prev = &list->tokens[curr - 1];
		if (prev->type == TOKEN_WORD) {
			if (prev->len == 1)
				return;
		}
	}

	t->sentence_end = true;
}

void tokenize(const char *string, TokenList **out) {
	if (!string) return;
	TokenList *list = token_list_init();
	if (!list) {
		*out = NULL;
		return;
	}

	const char *p = string;
	const char *end = string + strlen(string);
	
	while (p < end) {
		const char *token_start = p;

		// skip non-ASCII (UTF-8, emojis, ...)
		if ((unsigned char)*p > 127) {
			p++; 
			continue;
		}

		// whitespace 
		if (isspace(*p)) {
			while (p < end && isspace(*p)) 
				p++;
			// why add tokens once whitespace consumed??
			add_token(list, token_start, p, TOKEN_WHITESPACE);
			continue;
		}
		
		// hashtag
		if (*p == '#' && p + 1 < end && isalnum(*(p + 1))) {
			p++; 
			while (p < end && (isalnum(*p) || *p == '_'))
				p++;
			add_token(list, token_start, p, TOKEN_HASHTAG);
			continue;
		}

		// mention
		if (*p == '@' && p + 1 < end && isalnum(*(p + 1))) {
			p++;
			while (p < end && (isalnum(*p) || *p == '_'))
				p++;
			add_token(list, token_start, p, TOKEN_MENTION);
			continue;
		}

		// URL (skipped for now) 
		if ((strncmp(p, "http://", 7) == 0) ||
        		(strncmp(p, "https://", 8) == 0) ||
        		(strncmp(p, "www.", 4) == 0)) {
        			while (p < end && is_url_char(*p))
					p++; 
        			continue; 
		}
				
		// Number or Time 
		if (isdigit(*p)) {
			const char *start = p;
			// alphanumeric check (e.g. CAs)
			if (p + 1 < end && isalpha(*(p+1))) {
				p++;
				while (p < end && isalnum(*p)) 
					p++;
				add_token(list, start, p, TOKEN_WORD);
				continue;
			}

			int digit_count = 0;
			while (p < end && isdigit(*p) && digit_count < 2) {
				p++;
				digit_count++;
			}
			// time check 
			if (p < end && *p == ':' && p + 2 < end && isdigit(*(p + 1))) {
				p++; 
				if (isdigit(*p)) 
					p++;
				if (p < end && isdigit(*p))
					p++;

				// am/pm check -- handles both 00:00am and 00:00 am 
				while (p < end && isspace((unsigned char)*p)) 
					p++;
				
				if (p + 1 < end && (tolower(*p) == 'a' || tolower(*p) == 'p') 
						&& tolower(*(p+1)) == 'm') {
					p += 2;
				}
				
				add_token(list, start, p, TOKEN_TIME);
				continue;
			}
			// number check 
			p = start;
			while (p < end && (isdigit(*p) || *p == '.' || *p == ',')) 
				p++;
			add_token(list, start, p, TOKEN_NUMBER);
			continue;
		}
		
		// Alphabetic
		if (isalpha(*p)) {
			while (p < end && (isalnum(*p) || *p == '\'' || *p == '-'))
				p++;
			add_token(list, token_start, p, TOKEN_WORD);
			continue;
		}

		// Punctuation
		if (ispunct(*p)) {
			p++;
			add_token(list, token_start, p, TOKEN_PUNCT);
			// check for sentence boundaries on each punctuation
			sentence_bounds(list, end);
			continue;
		}

		
		// Date (to be implemented, expect low frequency)

		// Unknown
		p++;
		add_token(list, token_start, p, TOKEN_UNKNOWN);
	}

	*out = list;
}

#define MAX_SENTENCE_LEN 24

void tokenize_sentence(const TokenList *in, TokenList **out) {
	if (!in || in->size == 0) {
		*out = NULL;
		return;
	} 

	TokenList *sentences = malloc(sizeof(TokenList));
	if (!sentences) { *out = NULL; return; }

	sentences->tokens = malloc(sizeof(Token) * MAX_SENTENCE_LEN);
	if (!sentences->tokens) { free(sentences); *out = NULL; return; }

	sentences->size = 0; 
	sentences->capacity = MAX_SENTENCE_LEN;

	size_t start_idx = 0;

	for (size_t i = 0; i < in->size; i++) {
		Token *token = &in->tokens[i];
		// if sentence end, get last known 'start_idx' of TokenList until curr token end to obtain one sentence 
		// update start idx to curr idx, next sentence_end will count backwards to this new start idx to obtain the 
		// next sentence
		if (token->sentence_end) {
			// grab first token 
			const char *start = in->tokens[start_idx].start;
			// grab last token 
			const char *end = token->end;

			// ensure no realloc needed 
			if (sentences->size >= sentences->capacity) {
				size_t new_cap = sentences->capacity * 2;
				Token *tmp = realloc(sentences->tokens, sizeof(Token) * new_cap);
				if (!tmp) { perror("realloc"); break; }
				sentences->tokens = tmp;
				sentences->capacity = new_cap;
			}

			Token *sentence_tok = &sentences->tokens[sentences->size++];
			sentence_tok->start = start;
			sentence_tok->end = end;
			sentence_tok->len = end - start;
			sentence_tok->type = TOKEN_UNKNOWN;
			sentence_tok->sentence_end = true;
			start_idx = i + 1;
		}
	}

	// handle trailing tokens 
	if (start_idx < in->size) {
		const char *start = in->tokens[start_idx].start;
		const char *end = in->tokens[in->size - 1].end;

		if (sentences->size >= sentences->capacity) {
			size_t new_cap = sentences->capacity * 2;
			Token *tmp = realloc(sentences->tokens, sizeof(Token) * new_cap);
			if (tmp) { sentences->tokens = tmp; sentences->capacity = new_cap; }
		}

		Token *reg_tok = &sentences->tokens[sentences->size++];
		reg_tok->start = start;
		reg_tok->end = end;
		reg_tok->len = start - end;
		reg_tok->type = TOKEN_UNKNOWN;
		reg_tok->sentence_end = true;
	}
	*out = sentences; 
}

void print_tokens(TokenList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->size; i++) {
        Token *t = &list->tokens[i];
        printf("[%.*s]", (int)(t->end - t->start), t->start);
        if (t->sentence_end) 
            printf("<S>");   // mark sentence boundary
        printf(" ");
    }
    putchar('\n');
}

void print_sentence_tokens(TokenList *list) {
	if (!list) return;
	for (size_t i = 0; i < list->size; i++) {
		Token *sentence = &list->tokens[i];
		printf("[%.*s]<S> ", (int)(sentence->end - sentence->start), sentence->start);
	} 
	putchar('\n');
}

void free_tokens(TokenList *list) {
	if (!list) return;
	free(list->tokens);
	free(list);
}

/* Test 
int main(void) {
	const char *t = "Hi, my name is John. I am from America.";
	TokenList *list = NULL;
	tokenize(t, &list);

	TokenList *sentence_tokenization = NULL;
	tokenize_sentence(list, &sentence_tokenization);
	print_sentence_tokens(sentence_tokenization);

	putchar('\n');
	free_tokens(list);
	free_tokens(sentence_tokenization);
}*/
