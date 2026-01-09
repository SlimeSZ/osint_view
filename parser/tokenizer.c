#include "tokenizer.h" 
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
		// understand this realloc computation
		Token *new_token = realloc(list->tokens, list->capacity * sizeof(Token));
		if (!new_token) return;
		list->tokens = new_token;
	}
	size_t n = list->size;
	list->tokens[n].start = start;
	list->tokens[n].end = end;
	list->tokens[n].len = end - start;
	list->tokens[n].type = type;
	list->size++;
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

		// URL (to be implemented) 
		
		// Number
		if (isdigit(*p)) {
			while (p < end && (isdigit(*p) || *p == '.' || *p == ','))
				p++;
			add_token(list, token_start, p, TOKEN_NUMBER);
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
			continue;
		}

		// Unknown
		p++;
		add_token(list, token_start, p, TOKEN_UNKNOWN);
	}

	*out = list;
}

void print_tokens(TokenList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->size; i++) {
        Token *t = &list->tokens[i];
        printf("[%.*s] ", (int)(t->end - t->start), t->start);
    }
    putchar('\n');
}


void free_tokens(TokenList *list) {
	if (!list) return;
	free(list->tokens);
	free(list);
}


int main(void) {
	const char *t = "DHS statement to @FoxNews “At \
	2:19 PST, US Border Patrol agents were conducting a targeted vehicle stop in Portland, Oregon. The passenger of the vehicle and target is a Venezuelan illegal alien affiliated with the transnational Tren de Aragua prostitution ring and involved in a recent shooting in Portland. The vehicle driver is believed to be a member of the vicious Venezuelan gang Tren de Aragua. When agents identified themselves to the vehicle occupants, the driver weaponized his vehicle and attempted to run over the law enforcement agents. Fearing for his life and safety, an agent fired a defensive shot. The driver drove off with the passenger, fleeing the scene. This situation is evolving and more information is forthcoming.”";
	TokenList *list = NULL;
	tokenize(t, &list);
	print_tokens(list);
	putchar('\n');
	free_tokens(list);

	return 0;
}


