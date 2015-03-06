#ifndef PARSER_H
#define PARSER_H

#include "astree.h"
#include "lexer.h"

int parse(lexer_t* lexbuf, ASTreeNode** syntax_tree);

#endif