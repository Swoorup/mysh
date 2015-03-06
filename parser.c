#include "parser.h"
#include "lexer.h"
#include "astree.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*** Shell Grammer as given in the assignment question 1 ***/

/**
 *
	<command line>	::=  	<job>
						|	<job> '&'
						| 	<job> '&' <command line>
						|	<job> ';'
						|	<job> ';' <command line>

	<job>			::=		<command>
						|	< job > '|' < command >

	<command>		::=		<simple command>
						|	<simple command> '<' <filename>
						|	<simple command> '>' <filename>

	<simple command>::=		<pathname>
						|	<simple command>  <token>
 *
 *
 *
**/

/*** Shell Grammer for recursive descent parser ***/
/*** Removed left recursion and left factoring ***/

/**
 *
	<command line>	::= 	<job> ';' <command line>
						|	<job> ';'
						| 	<job> '&' <command line>
						|	<job> '&'
							<job>

	<job>			::=		<command> '|' <job>
						|	<command>

	<command>		::=		<simple command> '<' <filename> // this grammer is a bit incorrect, see grammer.llf
						|	<simple command> '>' <filename>
						|	<simple command>

	<simple command>::=		<pathname> <token list>

	<token list>	::=		<token> <token list>
						|	(EMPTY)

 *
 *
 *
**/

ASTreeNode* CMDLINE();		//	test all command line production orderwise
ASTreeNode* CMDLINE1();		//	<job> ';' <command line>
ASTreeNode* CMDLINE2();		//	<job> ';'
ASTreeNode* CMDLINE3();		//	<job> '&' <command line>
ASTreeNode* CMDLINE4();		//	<job> '&'
ASTreeNode* CMDLINE5();		//	<job>

ASTreeNode* JOB();			// test all job production in order
ASTreeNode* JOB1();			// <command> '|' <job>
ASTreeNode* JOB2();			// <command>

ASTreeNode* CMD();			// test all command production orderwise
ASTreeNode* CMD1();			//	<simple command> '<' <filename>
ASTreeNode* CMD2();			//	<simple command> '>' <filename>
ASTreeNode* CMD3();			//	<simple command>

ASTreeNode* SIMPLECMD();	// test simple cmd production
ASTreeNode* SIMPLECMD1();	// <pathname> <token list>

ASTreeNode* TOKENLIST();	// test tokenlist production
ASTreeNode* TOKENLIST1();	//	<token> <token list>
ASTreeNode* TOKENLIST2();	//	EMPTY

// curtok token pointer
tok_t* curtok = NULL;

bool term(int toketype, char** bufferptr)
{
	if (curtok == NULL)
		return false;
	
    if (curtok->type == toketype)
    {
		if (bufferptr != NULL) {
			*bufferptr = malloc(strlen(curtok->data) + 1);
			strcpy(*bufferptr, curtok->data);
		}
		curtok = curtok->next;
        return true;
    }

    curtok = curtok->next;
    return false;
}

ASTreeNode* CMDLINE()
{
    tok_t* save = curtok;

    ASTreeNode* node;

    if ((curtok = save, node = CMDLINE1()) != NULL)
        return node;

    if ((curtok = save, node = CMDLINE2()) != NULL)
        return node;

    if ((curtok = save, node = CMDLINE3()) != NULL)
        return node;

    if ((curtok = save, node = CMDLINE4()) != NULL)
        return node;

    if ((curtok = save, node = CMDLINE5()) != NULL)
        return node;

    return NULL;
}

ASTreeNode* CMDLINE1()
{
    ASTreeNode* jobNode;
    ASTreeNode* cmdlineNode;
    ASTreeNode* result;

    if ((jobNode = JOB()) == NULL)
        return NULL;

    if (!term(CHAR_SEMICOLON, NULL)) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    if ((cmdlineNode = CMDLINE()) == NULL) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_SEQ);
    ASTreeAttachBinaryBranch(result, jobNode, cmdlineNode);

    return result;
}

ASTreeNode* CMDLINE2()
{
    ASTreeNode* jobNode;
    ASTreeNode* result;

    if ((jobNode = JOB()) == NULL)
        return NULL;

	if (!term(CHAR_SEMICOLON, NULL)) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_SEQ);
    ASTreeAttachBinaryBranch(result, jobNode, NULL);

    return result;
}

ASTreeNode* CMDLINE3()
{
    ASTreeNode* jobNode;
    ASTreeNode* cmdlineNode;
    ASTreeNode* result;

    if ((jobNode = JOB()) == NULL)
        return NULL;

    if (!term(CHAR_AMPERSAND, NULL)) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    if ((cmdlineNode = CMDLINE()) == NULL) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_BCKGRND);
    ASTreeAttachBinaryBranch(result, jobNode, cmdlineNode);

    return result;
}

ASTreeNode* CMDLINE4()
{
    ASTreeNode* jobNode;
    ASTreeNode* result;

    if ((jobNode = JOB()) == NULL)
        return NULL;

	if (!term(CHAR_AMPERSAND, NULL)) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_BCKGRND);
    ASTreeAttachBinaryBranch(result, jobNode, NULL);

    return result;
}

ASTreeNode* CMDLINE5()
{
    return JOB();
}

ASTreeNode* JOB()
{
    tok_t* save = curtok;

    ASTreeNode* node;

    if ((curtok = save, node = JOB1()) != NULL)
        return node;

    if ((curtok = save, node = JOB2()) != NULL)
        return node;

    return NULL;
}

ASTreeNode* JOB1()
{
    ASTreeNode* cmdNode;
    ASTreeNode* jobNode;
    ASTreeNode* result;

    if ((cmdNode = CMD()) == NULL)
        return NULL;

    if (!term(CHAR_PIPE, NULL)) {
        ASTreeNodeDelete(cmdNode);
        return NULL;
    }

    if ((jobNode = JOB()) == NULL) {
        ASTreeNodeDelete(cmdNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_PIPE);
    ASTreeAttachBinaryBranch(result, cmdNode, jobNode);

    return result;
}

ASTreeNode* JOB2()
{
    return CMD();
}

ASTreeNode* CMD()
{
    tok_t* save = curtok;

    ASTreeNode* node;

    if ((curtok = save, node = CMD1()) != NULL)
        return node;

    if ((curtok = save, node = CMD2()) != NULL)
        return node;

    if ((curtok = save, node = CMD3()) != NULL)
        return node;

    return NULL;
}

ASTreeNode* CMD1()
{
    ASTreeNode* simplecmdNode;
    ASTreeNode* result;

    if ((simplecmdNode = SIMPLECMD()) == NULL)
        return NULL;

    if (!term(CHAR_LESSER, NULL)) {
		ASTreeNodeDelete(simplecmdNode);
		return NULL;
	}
	
	char* filename;
	if (!term(TOKEN, &filename)) {
		free(filename);
        ASTreeNodeDelete(simplecmdNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_REDIRECT_IN);
    ASTreeNodeSetData(result, filename);
    ASTreeAttachBinaryBranch(result, NULL, simplecmdNode);

    return result;
}

ASTreeNode* CMD2()
{
    ASTreeNode* simplecmdNode;
    ASTreeNode* result;

    if ((simplecmdNode = SIMPLECMD()) == NULL)
        return NULL;

	if (!term(CHAR_GREATER, NULL)) {
		ASTreeNodeDelete(simplecmdNode);
		return NULL;
	}
	
	char* filename;
	if (!term(TOKEN, &filename)) {
		free(filename);
		ASTreeNodeDelete(simplecmdNode);
		return NULL;
	}

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_REDIRECT_OUT);
    ASTreeNodeSetData(result, filename);
	ASTreeAttachBinaryBranch(result, NULL, simplecmdNode);

    return result;
}

ASTreeNode* CMD3()
{
    return SIMPLECMD();
}

ASTreeNode* SIMPLECMD()
{
    tok_t* save = curtok;
    return SIMPLECMD1();
}

ASTreeNode* SIMPLECMD1()
{
    ASTreeNode* tokenListNode;
    ASTreeNode* result;

    char* pathname;
    if (!term(TOKEN, &pathname))
        return NULL;

    tokenListNode = TOKENLIST();
    // we don't check whether tokenlistNode is NULL since its a valid grammer

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_CMDPATH);
    ASTreeNodeSetData(result, pathname);
	ASTreeAttachBinaryBranch(result, NULL, tokenListNode);

    return result;
}

ASTreeNode* TOKENLIST()
{
    tok_t* save = curtok;

    ASTreeNode* node;

    if ((curtok = save, node = TOKENLIST1()) != NULL)
        return node;

    if ((curtok = save, node = TOKENLIST2()) != NULL)
        return node;

    return NULL;
}

ASTreeNode* TOKENLIST1()
{
    ASTreeNode* tokenListNode;
    ASTreeNode* result;

    char* arg;
    if (!term(TOKEN, &arg))
        return NULL;

    tokenListNode = TOKENLIST();
    // we don't check whether tokenlistNode is NULL since its a valid grammer

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_ARGUMENT);
    ASTreeNodeSetData(result, arg);
	ASTreeAttachBinaryBranch(result, NULL, tokenListNode);

    return result;
}

ASTreeNode* TOKENLIST2()
{
    return NULL;
}

int parse(lexer_t* lexbuf, ASTreeNode** syntax_tree)
{
	if (lexbuf->ntoks == 0)
		return -1;
	
	curtok = lexbuf->llisttok;
    *syntax_tree = CMDLINE();
	
    if (curtok != NULL && curtok->type != 0)
    {
        printf("Syntax Error near: %s\n", curtok->data);
        return -1;
    }
	
	return 0;
}
