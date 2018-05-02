#include <glob.h>
#include <string.h>
#include <stdlib.h>
#include "lexer.h"

int getchartype(char c)
{
	switch(c)
	{
		case '\'':
			return CHAR_QOUTE;
			break;
		case '\"':
			return CHAR_DQUOTE;
			break;
		case '|':
			return CHAR_PIPE;
			break;
		case '&':
			return CHAR_AMPERSAND;
			break;
		case ' ':
			return CHAR_WHITESPACE;
			break;
		case ';':
			return CHAR_SEMICOLON;
			break;
		case '\\':
			return CHAR_ESCAPESEQUENCE;
			break;
		case '\t':
			return CHAR_TAB;
			break;
		case '\n':
			return CHAR_NEWLINE;
			break;
		case '>':
			return CHAR_GREATER;
			break;
		case '<':
			return CHAR_LESSER;
			break;
		case 0:
			return CHAR_NULL;
			break;
		default:
			return CHAR_GENERAL;
	};
	
	return CHAR_GENERAL;
}

void strip_quotes(char* src, char* dest)
{
	int n = strlen(src);
	if (n <= 1) {
		strcpy(dest, src);
		return;
	}
	
	int i;
	
	char lastquote = 0;
	int j = 0;
	
	for (i=0; i < n; i++)
	{
		char c = src[i];
		if ((c == '\'' || c == '\"') && lastquote == 0)
			lastquote = c;
		else if (c == lastquote)
			lastquote = 0;
		else
			dest[j++] = c;
	}
	
	dest[j] = 0;
}

void tok_init(tok_t* tok, int datasize)
{
	tok->data = malloc(datasize + 1); // 1 for null terminator
	tok->data[0] = 0;
	
	tok->type = CHAR_NULL;
	tok->next = NULL;
}

void tok_destroy(tok_t* tok) {
	if (tok != NULL) {
		free(tok->data);
		tok_destroy(tok->next);
		free(tok);
	}
}

int lexer_build(char* input, int size, lexer_t* lexerbuf)
{
	if (lexerbuf == NULL)
		return -1;
	
	if (size == 0) {
		lexerbuf->ntoks = 0;
		return 0;
	}
	
	lexerbuf->llisttok = malloc(sizeof(tok_t));
	
	// allocate the first token
	tok_t* token = lexerbuf->llisttok;
	tok_init(token, size);
	
	int i = 0;
	int j = 0, ntemptok = 0;
	
	char c;
	int state = STATE_GENERAL;
	
	do
	{
		c = input[i];		
		int chtype = getchartype(c);
		
		if (state == STATE_GENERAL)
		{
			switch (chtype) 
			{
				case CHAR_QOUTE:
					state = STATE_IN_QUOTE;
					token->data[j++] = CHAR_QOUTE;
					token->type = TOKEN;
					break;
					
				case CHAR_DQUOTE:
					state = STATE_IN_DQUOTE;
					token->data[j++] = CHAR_DQUOTE;
					token->type = TOKEN;
					break;
					
				case CHAR_ESCAPESEQUENCE:
					token->data[j++] = input[++i];
					token->type = TOKEN;
					break;
					
				case CHAR_GENERAL:
					token->data[j++] = c;
					token->type = TOKEN;
					break;
					
				case CHAR_WHITESPACE:
					if (j > 0) {
						token->data[j] = 0;
						token->next = malloc(sizeof(tok_t));
						token = token->next;
						tok_init(token, size - i);
						j = 0;
					}
					break;
					
				case CHAR_SEMICOLON:
				case CHAR_GREATER:
				case CHAR_LESSER:
				case CHAR_AMPERSAND:
				case CHAR_PIPE:
					
					// end the token that was being read before
					if (j > 0) {
						token->data[j] = 0;
						token->next = malloc(sizeof(tok_t));
						token = token->next;
						tok_init(token, size - i);
						j = 0;
					}
					
					// next token
					token->data[0] = chtype;
					token->data[1] = 0;
					token->type = chtype;
					
					token->next = malloc(sizeof(tok_t));
					token = token->next;
					tok_init(token, size - i);
					break;
			}
		}
		else if (state == STATE_IN_DQUOTE) {
			token->data[j++] = c;
			if (chtype == CHAR_DQUOTE)
				state = STATE_GENERAL;
			
		}
		else if (state == STATE_IN_QUOTE) {
			token->data[j++] = c;
			if (chtype == CHAR_QOUTE)
				state = STATE_GENERAL;
		}
		
		if (chtype == CHAR_NULL) {
			if (j > 0) {
				token->data[j] = 0;
				ntemptok++;
				j = 0;
			}
		}
		
		i++;
	} while (c != '\0');
	
	token = lexerbuf->llisttok;
	int k = 0;
	while (token != NULL) 
	{
		if (token->type == TOKEN)
		{
			glob_t globbuf;
			glob(token->data, GLOB_TILDE, NULL, &globbuf);
			
			if (globbuf.gl_pathc > 0)
			{
				k += globbuf.gl_pathc;
				// save the next token so we can attach it later
				tok_t* saved = token->next;
				
				// replace the current token with the first one
				free(token->data);
				token->data = malloc(strlen(globbuf.gl_pathv[0]) + 1);
				strcpy(token->data, globbuf.gl_pathv[0]);
								
				int i;
				for (i = 1; i < globbuf.gl_pathc; i++)
				{
					token->next = malloc(sizeof(tok_t));
					tok_init(token->next, strlen(globbuf.gl_pathv[i]));
					token = token->next;
					token->type = TOKEN;
					strcpy(token->data, globbuf.gl_pathv[i]);
				}
				
				token->next = saved;
			}
			else {
				// token from the user might be inside quotation to escape special characters
				// hence strip the quotation symbol
				char* stripped = malloc(strlen(token->data) + 1);
				strip_quotes(token->data, stripped);
				free(token->data);
				token->data = stripped;
				k++;
			}
		}
		
		token = token->next;
	}
	
	lexerbuf->ntoks = k;
	return k;
}

void lexer_destroy(lexer_t* lexerbuf)
{
	if (lexerbuf == NULL)
		return;
	
	tok_destroy(lexerbuf->llisttok);
}
