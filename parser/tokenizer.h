#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
	TOKEN_WORD,
	TOKEN_NUMBER,
	TOKEN_PUNCT, // .,!?; etc..
	TOKEN_DATE,
	TOKEN_TIME, 
	TOKEN_WHITESPACE,
	TOKEN_HASHTAG,
	TOKEN_MENTION,
	TOKEN_URL,
	TOKEN_SYMBOL, // $, %, &, etc.. (special char.)
	TOKEN_UNKNOWN // fallback 
} TokenType;

typedef struct {
    const char *start;   // pointer to token start
    const char *end;     // pointer to token end
    size_t len;          // length in chars
    TokenType type;      // token type 
    bool sentence_end;
} Token;

typedef struct {
	Token *tokens;
	size_t size;
	size_t capacity;
} TokenList;

// main API 
void tokenize(const char *string, TokenList **out);
void print_tokens(TokenList *list);
void free_tokens(TokenList *list);

#endif
