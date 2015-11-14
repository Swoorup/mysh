# mysh
A very simply sh-like unix shell written in C using recursive descent parser technique.

# WHY
Last summer, I started out writing a shell for my assignment. Thought I'd share it as well. I'll explain this in conjunction with the project hosted in [GitHub](https://github.com/Swoorup/mysh/blob/master/lexer.c#L97). Please download it and have your favourite editor ready or you might not be able to make any single sense out of this.

I am sure you may have come across using a shell. You have used command prompt in windows, or the popular bash in Linux. But how do they work? Lets see if we can break it down.

Without further ado, lets start at the concepts. Now the shell consists of number of rules which we take as granted. And by these rules I meant think of it as a computer working on your high school algebra.

`E.g: (1+2 % 4 x (1 + 1))`

It would have to work out the BDMAS (brackets first, then divide, multiply, addition and subtraction respectively) rule here. Think of the possibilities of the user input first. You first need to make sure that the input is valid and error-free (You don't want your device to do unexpected things when the user enter unexpected input). You would then need to break down each individual tokens into an ordered list, check if there are any errors in the syntax and then try and parse them into an execution tree. If you want your computer to solve something like this, its obvious it would need more than a page worth of code. You can off-course do it in many ways, but its better to stick to a well known structure.

# Brief Summary

Now I wont dive into deeper into every aspects of the shell code. I'll just highlight the main concepts.  We start at the entry point of the program. Since I am programming this for *nix system, we would have to get or redirect the unix signals like Ctrl-Z, Ctrl-C and Ctrl-\\.

```c
int main()
{
    // ignore Ctrl-\ Ctrl-C Ctrl-Z signals
    ignore_signal_for_shell();

    // set the prompt
    set_prompt("swoorup % ");

    while (1)
```

Then there is a main loop which would frequently scan for commands, parse it into token, check the grammar and syntax and either produce an error or an output the resulting command.

```c
// keep getline in a loop in case interruption occurs
int again = 1;
while (again) {
	again = 0;
	printf("%s", getprompt());
	linebuffer = NULL;
	len = 0;
	ssize_t nread = getline(&linebuffer, &len, stdin);
	if (nread <= 0 && errno == EINTR) {
		again = 1;        	// signal interruption, read again
		clearerr(stdin);	// clear the error
	}
}

// user pressed ctrl-D
if (feof(stdin)) {
	exit(0);
	return 0;
}

// lexically analyze and build a list of tokens
lexer_build(linebuffer, len, &lexerbuf);
```

Until now, we have scanned the user input, have broken them down into array of lexis or tokens and now we try and run them through a parser which parses them into an [Abstract Syntax Tree](https://en.wikipedia.org/wiki/Abstract_syntax_tree). If there is an error in the user grammar, the parse will throw out an error.

```c
// parse the tokens into an abstract syntax tree
if (!lexerbuf.ntoks || parse(&lexerbuf, &exectree) != 0) {
	continue;
}
execute_syntax_tree(exectree);
```

# The Lexical Analyzer

The lexical analysis is probably the most simplest, yet an important part of the shell. All it does is simply break the input string from the user to a series of tokens. Now tokens could either be a character or a series of characters. It also groups characters that are inside a single or double quotations.

It is also able to expand wildcard characters. This is done via simply using the [glob ](http://linux.die.net/man/3/glob)POSIX function. It is probably better to have a look at the [lexer_build](https://github.com/Swoorup/mysh/blob/master/lexer.c#L97) function in lexer.c

# The Syntax Tree Parser

This topic is probably the most actively discussed topic in compiler theory and designs. In this case, I have a very simply implementation based on recursive parsing technique.

After we get the tokens from the lexer, we feed them to the parser or the syntax tree builder. This is done entirely in [parser.c](https://github.com/Swoorup/mysh/blob/master/parser.c#L418). For those who do not know what a syntax tree is, it is the most important part of the compiler/interpreter. Put simply, it is a binary tree-like data structure that holds tokens and operations in order of the execution. Now, there are automatic ways to actually define the whole functions that parses texts in syntax tree. In this one, I have implemented it manually for the sake of simplicity.

The shell language grammer is defined as follows in [Backus–Naur form](https://en.wikipedia.org/wiki/Backus%E2%80%93Naur_form):

```c
<command line>	::	<job>
		|	<job> '&'
		| 	<job> '&' <command line>
		|	<job> ';'
		|	<job> ';' <command line>
					         
<job>		::=	<command>
		|	< job > '|' < command >
					        
<command	::=	<simple command>
	        |	<simple command> '<' <filename>
	        |	<simple command> '>' <filename>
					        
<simple command>::=	<pathname>
	        |	<simple command>  <token>
```


You can observe in parser.c that for all the productions in the grammer, set of functions are defined which validates each rules along with the main function that calls those set for the equivalent production. The purpose is to recursively check if the order of tokens belongs to a particular grammer. This reduces down to check for terminal symbols and non-terminals. In case  of a non-terminal the function sets equivalent to that production is simply called.

Our topmost grammer is the command-line grammer. So in the parse function, we first checks if the list of tokens in the sequential order is a command-line production by calling CMDLINE() which tests all its rules.

The astree functions which is used throught parser.c simply define the data structure for a binary tree.

```c

typedef struct ASTreeNode
{
    int type;
    char* szData;
    struct ASTreeNode* left;
    struct ASTreeNode* right;

} ASTreeNode;
```
# Execution

After we build our syntax tree, it is quite easy to traverse through out the tokens in order and make executions if necessary. We are certain that our syntax tree works because the parser would throw an error in the first place if it wasn't. The syntax tree is executed from [execute_syntax_tree](https://github.com/Swoorup/mysh/blob/master/execute.c#L139) function in execute.c. I hope its self-explanatary enough as adding some more explanations here would simply be a burden to read. :P

Phew! Well I hope that was informative.

-Swoorup
