#ifndef COMMAND_H
#define COMMAND_H

#include <unistd.h>
#include <stdbool.h>
#include "astree.h"

struct CommandInternal
{
	int argc;
	char **argv;
	bool stdin_pipe;
	bool stdout_pipe;
	int pipe_read;
	int pipe_write;
	char* redirect_in;
	char* redirect_out;
	bool asynchrnous;
};

typedef struct CommandInternal CommandInternal;

void set_prompt(char* str);
char* getprompt();
void ignore_signal_for_shell();
void execute_cd(CommandInternal* cmdinternal);
void execute_prompt(CommandInternal* cmdinternal);
void execute_pwd(CommandInternal* cmdinternal);
void execute_command_internal(CommandInternal* cmdinternal);
int init_command_internal(ASTreeNode* simplecmdNode, 
						  CommandInternal* cmdinternal, 
						  bool async,
						  bool stdin_pipe,
						  bool stdout_pipe,
						  int pipe_read,
						  int pipe_write,
						  char* redirect_in,
						  char* redirect_out
);
void destroy_command_internal(CommandInternal* cmdinternal);

#endif
