#ifndef LEXER_H
#define LEXER_H

enum TokenType{
	CHAR_GENERAL = -1,
	CHAR_PIPE = '|',
	CHAR_AMPERSAND = '&',
	CHAR_QOUTE = '\'',
	CHAR_DQUOTE = '\"',
	CHAR_SEMICOLON = ';',
	CHAR_WHITESPACE = ' ',
	CHAR_ESCAPESEQUENCE = '\\',
	CHAR_TAB = '\t',
	CHAR_NEWLINE = '\n',
	CHAR_GREATER = '>',
	CHAR_LESSER = '<',
	CHAR_NULL = 0,
	
	TOKEN	= -1,
};

enum {
	STATE_IN_DQUOTE,
	STATE_IN_QUOTE,
	
	STATE_IN_ESCAPESEQ,
	STATE_GENERAL,
};

typedef struct tok tok_t;
typedef struct lexer lexer_t;

struct tok {
	char* data;
	int type;
	tok_t* next;
};

struct lexer
{
	tok_t* llisttok;
	int ntoks;
};

int lexer_build(char* input, int size, lexer_t* lexerbuf);
void lexer_destroy(lexer_t* lexerbuf);
#endif
