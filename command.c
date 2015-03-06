#include "command.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <pwd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

char* prompt = NULL;
bool signalset = false;
void   (*SIGINT_handler)(int);

void set_prompt(char* str)
{
    if (prompt != NULL)
        free(prompt);

    prompt = malloc(strlen(str) + 1);
    strcpy(prompt, str);
}

char* getprompt()
{
	return prompt;
}

void ignore_signal_for_shell()
{
	signalset = true;
	
	// ignore "Ctrl-C"
    SIGINT_handler = signal(SIGINT, SIG_IGN);
	// ignore "Ctrl-Z"
    signal(SIGTSTP, SIG_IGN);
	// ignore "Ctrl-\"
    signal(SIGQUIT, SIG_IGN);
}

// restore Ctrl-C signal in the child process
void restore_sigint_in_child()
{
	if (signalset)
		signal(SIGINT, SIGINT_handler);
}

// built-in command cd
void execute_cd(CommandInternal* cmdinternal)
{
    if (cmdinternal->argc == 1) {
		struct passwd *pw = getpwuid(getuid());
		const char *homedir = pw->pw_dir;
		chdir(homedir);
	}
    else if (cmdinternal->argc > 2)
        printf("cd: Too many arguments\n");
    else {
        if (chdir(cmdinternal->argv[1]) != 0)
            perror(cmdinternal->argv[1]);
    }
}

// built-in command prompt
void execute_prompt(CommandInternal* cmdinternal)
{
    if (cmdinternal->argc == 1)
        printf("prompt: Please specify the prompt string\n");
    else if (cmdinternal->argc > 2)
        printf("prompt: Too many arguments\n");
    else {
        set_prompt(cmdinternal->argv[1]);
    }
}

// built-in command pwd
void execute_pwd(CommandInternal* cmdinternal)
{
    pid_t pid;
    if((pid = fork()) == 0 ) {
        if (cmdinternal->redirect_out) {
            int fd = open(cmdinternal->redirect_out, O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd == -1) {
                perror(cmdinternal->redirect_out);
                exit(1);
            }

            dup2(fd, STDOUT_FILENO);
        }

        if (cmdinternal->stdout_pipe)
			dup2(cmdinternal->pipe_write, STDOUT_FILENO);

        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            fprintf(stdout, "%s\n", cwd);
        else
            perror("getcwd() error");

        exit(0);
    }
    else if (pid < 0) {
        perror("fork");
        return;
    }
    else
		while (waitpid(pid, NULL, 0) <= 0);

    return;
}

void zombie_process_handler(int signum)
{
    int more = 1;        // more zombies to claim
    pid_t pid;           // pid of the zombie
    int status;          // termination status of the zombie

    while (more) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0)
            printf("\n%d terminated\n", pid);
        if (pid<=0)
            more = 0;
    }
}

void execute_command_internal(CommandInternal* cmdinternal)
{

    if (cmdinternal->argc < 0)
        return;

    // check for built-in commands
    if (strcmp(cmdinternal->argv[0], "cd") == 0) {
        execute_cd(cmdinternal);
        return;
    }
    else if (strcmp(cmdinternal->argv[0], "prompt") == 0) {
		execute_prompt(cmdinternal);
        return;
	}
    else if (strcmp(cmdinternal->argv[0], "pwd") == 0)
        return execute_pwd(cmdinternal);
    else if(strcmp(cmdinternal->argv[0], "exit") == 0) {
		exit(0);
		return;
	}

    pid_t pid;
    if((pid = fork()) == 0 ) {
		// restore the signals in the child process
		restore_sigint_in_child();
		
		// store the stdout file desc
        int stdoutfd = dup(STDOUT_FILENO);

		// for bckgrnd jobs redirect stdin from /dev/null
        if (cmdinternal->asynchrnous) {
            int fd = open("/dev/null",O_RDWR);
            if (fd == -1) {
                perror("/dev/null");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
        }

        // redirect stdin from file if specified
        if (cmdinternal->redirect_in) {
            int fd = open(cmdinternal->redirect_in, O_RDONLY);
            if (fd == -1) {
                perror(cmdinternal->redirect_in);
                exit(1);
            }

            dup2(fd, STDIN_FILENO);
        }

        // redirect stdout to file if specified
        else if (cmdinternal->redirect_out) {
            int fd = open(cmdinternal->redirect_out, O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd == -1) {
                perror(cmdinternal->redirect_out);
                exit(1);
            }

            dup2(fd, STDOUT_FILENO);
        }

        // read stdin from pipe if present
        if (cmdinternal->stdin_pipe)
            dup2(cmdinternal->pipe_read, STDIN_FILENO);

		// write stdout to pipe if present
        if (cmdinternal->stdout_pipe)
            dup2(cmdinternal->pipe_write, STDOUT_FILENO);

        if (execvp(cmdinternal->argv[0], cmdinternal->argv) == -1) {
			// restore the stdout for displaying error message
            dup2(stdoutfd, STDOUT_FILENO);
			
            printf("Command not found: \'%s\'\n", cmdinternal->argv[0]);
			exit(1);
        }

        
    }
    else if (pid < 0) {
        perror("fork");
        return;
    }

    if (!cmdinternal->asynchrnous)
    {
        // wait till the process has not finished
        while (waitpid(pid, NULL, 0) <= 0);
    }
    else
    {
		// set the sigchild handler for the spawned process
        printf("%d started\n", pid);
        struct sigaction act;
        act.sa_flags = 0;
        act.sa_handler = zombie_process_handler;
        sigfillset( & (act.sa_mask) ); // to block all

        if (sigaction(SIGCHLD, &act, NULL) != 0)
            perror("sigaction");
    }

    return;
}

int init_command_internal(ASTreeNode* simplecmdNode,
                          CommandInternal* cmdinternal,
                          bool async,
                          bool stdin_pipe,
                          bool stdout_pipe,
                          int pipe_read,
                          int pipe_write,
                          char* redirect_in,
                          char* redirect_out)
{
    if (simplecmdNode == NULL || !(NODETYPE(simplecmdNode->type) == NODE_CMDPATH))
    {
        cmdinternal->argc = 0;
        return -1;
    }

    ASTreeNode* argNode = simplecmdNode;

    int i = 0;
    while (argNode != NULL && (NODETYPE(argNode->type) == NODE_ARGUMENT || NODETYPE(argNode->type) == NODE_CMDPATH)) {
        argNode = argNode->right;
        i++;
    }

    cmdinternal->argv = (char**)malloc(sizeof(char*) * (i + 1));
    argNode = simplecmdNode;
    i = 0;
    while (argNode != NULL && (NODETYPE(argNode->type) == NODE_ARGUMENT || NODETYPE(argNode->type) == NODE_CMDPATH)) {
        cmdinternal->argv[i] = (char*)malloc(strlen(argNode->szData) + 1);
        strcpy(cmdinternal->argv[i], argNode->szData);

        argNode = argNode->right;
        i++;
    }

    cmdinternal->argv[i] = NULL;
    cmdinternal->argc = i;

    cmdinternal->asynchrnous = async;
    cmdinternal->stdin_pipe = stdin_pipe;
    cmdinternal->stdout_pipe = stdout_pipe;
    cmdinternal->pipe_read = pipe_read;
    cmdinternal->pipe_write = pipe_write;
    cmdinternal->redirect_in = redirect_in;
    cmdinternal->redirect_out = redirect_out;

    return 0;
}

void destroy_command_internal(CommandInternal* cmdinternal)
{
    int i;
    for (i = 0; i < cmdinternal->argc; i++)
        free(cmdinternal->argv[i]);

    free(cmdinternal->argv);
    cmdinternal->argc = 0;
}
