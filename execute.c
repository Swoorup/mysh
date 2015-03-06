#include "command.h"
#include <unistd.h>
#include <stdbool.h>

void execute_simple_command(ASTreeNode* simple_cmd_node,
                             bool async,
                             bool stdin_pipe,
                             bool stdout_pipe,
                             int pipe_read,
                             int pipe_write,
                             char* redirect_in,
                             char* redirect_out
                            )
{
    CommandInternal cmdinternal;
    init_command_internal(simple_cmd_node, &cmdinternal, async, stdin_pipe, stdout_pipe,
                          pipe_read, pipe_write, redirect_in, redirect_out
                         );
	execute_command_internal(&cmdinternal);
	destroy_command_internal(&cmdinternal);
}

void execute_command(ASTreeNode* cmdNode,
                      bool async,
                      bool stdin_pipe,
                      bool stdout_pipe,
                      int pipe_read,
                      int pipe_write)
{
    if (cmdNode == NULL)
        return;

    switch (NODETYPE(cmdNode->type))
    {
    case NODE_REDIRECT_IN:		// right side contains simple cmd node
        execute_simple_command(cmdNode->right,
                               async,
                               stdin_pipe,
                               stdout_pipe,
                               pipe_read,
                               pipe_write,
                               cmdNode->szData, NULL
                              );
        break;
    case NODE_REDIRECT_OUT:		// right side contains simple cmd node
        execute_simple_command(cmdNode->right,
                               async,
                               stdin_pipe,
                               stdout_pipe,
                               pipe_read,
                               pipe_write,
                               NULL, cmdNode->szData
                              );
        break;
    case NODE_CMDPATH:
        execute_simple_command(cmdNode,
                               async,
                               stdin_pipe,
                               stdout_pipe,
                               pipe_read,
                               pipe_write,
                               NULL, NULL
                              );
        break;
    }
}

void execute_pipeline(ASTreeNode* t, bool async)
{
    int file_desc[2];

    pipe(file_desc);
    int pipewrite = file_desc[1];
    int piperead = file_desc[0];

	// read input from stdin for the first job
    execute_command(t->left, async, false, true, 0, pipewrite);
    ASTreeNode* jobNode = t->right;

    while (jobNode != NULL && NODETYPE(jobNode->type) == NODE_PIPE)
    {
        close(pipewrite); // close the write end
        pipe(file_desc);
        pipewrite = file_desc[1];

        execute_command(jobNode->left, async, true, true, piperead, pipewrite);
        close(piperead);
        piperead = file_desc[0];

        jobNode = jobNode->right;
    }

    piperead = file_desc[0];
    close(pipewrite);
	
	// write output to stdout for the last job
    execute_command(jobNode, async, true, false, piperead, 0);	// only wait for the last command if necessary
    close(piperead);
}

void execute_job(ASTreeNode* jobNode, bool async)
{
    if (jobNode == NULL)
        return;

    switch (NODETYPE(jobNode->type))
    {
    case NODE_PIPE:
        execute_pipeline(jobNode, async);
        break;
    case NODE_CMDPATH:
    default:
        execute_command(jobNode, async, false, false, 0, 0);
        break;
    }
}

void execute_cmdline(ASTreeNode* cmdline)
{
    if (cmdline == NULL)
        return;

    switch(NODETYPE(cmdline->type))
    {
    case NODE_SEQ:
        execute_job(cmdline->left, false);
        execute_cmdline(cmdline->right);
        break;

    case NODE_BCKGRND:
        execute_job(cmdline->left, true);  // job to be background
        execute_cmdline(cmdline->right);
        break;
    default:
        execute_job(cmdline, false);
    }
}

void execute_syntax_tree(ASTreeNode* tree)
{
	// interpret the syntax tree
    execute_cmdline(tree);
}
