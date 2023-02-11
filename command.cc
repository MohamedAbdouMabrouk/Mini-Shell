
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ctime>

#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
	if (_numberOfAvailableArguments == _numberOfArguments + 1)
	{
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **)realloc(_arguments,
									  _numberOfAvailableArguments * sizeof(char *));
	}

	_arguments[_numberOfArguments] = argument;

	// Add NULL argument at the end
	_arguments[_numberOfArguments + 1] = NULL;

	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc(_numberOfSimpleCommands * sizeof(SimpleCommand *));

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
	if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
	{
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
													_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
	}

	_simpleCommands[_numberOfSimpleCommands] = simpleCommand;
	_numberOfSimpleCommands++;
}

void Command::clear()
{
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			free(_simpleCommands[i]->_arguments[j]);
		}

		free(_simpleCommands[i]->_arguments);
		free(_simpleCommands[i]);
	}

	if (_outFile)
	{
		free(_outFile);
	}

	if (_inputFile)
	{
		free(_inputFile);
	}

	if (_errFile && _errFile != _outFile)
	{
		free(_errFile);
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void Command::print()
{

	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  -------------------------------------------------------------\n");
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		printf("  %-3d ", i);
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
		}
	}
	printf("\n\n");
	printf("  Output       Input        Error        Background\n");
	printf("  ------------ ------------ ------------ ------------\n");
	printf("  %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
		   _inputFile ? _inputFile : "default", _errFile ? _errFile : "default",
		   _background ? "YES" : "NO");
	printf("\n\n");
}

void Command::execute()
{

	// Don't do anything if there are no simple commands
	if (_numberOfSimpleCommands == 0)
	{
		prompt();
		return;
	}
	// Print contents of Command data structure
	print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec

	int defaultin = dup(0);
	int defaultout = dup(1);
	int defaulterr = dup(2);
	int outfd;
	int infd;
	int fdpipe[2];
	int pid;

	if (_inputFile)
	{
		infd = open(_inputFile, O_CREAT | O_RDONLY, 0666);
		if (infd < 0)
		{
			exit(2);
		}
	}

	if (_outFile)
	{
		switch (_ioModifier)
		{
		case 0: // >
			outfd = creat(_outFile, 0666);
			break;
		case 1: // >>
			outfd = open(_outFile, O_CREAT | O_APPEND | O_WRONLY, 0666);
			break;
		}
		if (outfd < 0)
		{
			exit(2);
		}
	}

	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		if (i == 0)
		{
			if (_inputFile)
			{
				dup2(infd, 0);
				close(infd);
			}
			else
				dup2(defaultin, 0);
		}
		else
		{
			dup2(fdpipe[0], 0);
			close(fdpipe[0]);
		}

		if (i == _numberOfSimpleCommands - 1)
		{
			if (_outFile)
			{
				dup2(outfd, 1);
				close(outfd);
			}
			else
			{
				dup2(defaultout, 1);
			}
			if (_errFile)
			{
				dup2(outfd, 2);
			}
		}
		else
		{
			if (pipe(fdpipe) == -1)
			{
				perror("pipe");
				exit(2);
			}
			dup2(fdpipe[1], 1);
			close(fdpipe[1]);
		}
		// change directory
		if (strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0)
		{
			if (chdir(_simpleCommands[0]->_arguments[1]) < 0)
			{
				chdir(getenv("HOME"));
			}
			continue;
		}
		// fork
		pid = fork();
		if (pid == -1)
		{
			exit(2);
		}
		if (pid == 0)
		{
			// Child
			//  You can use execvp() instead if the arguments are stored in an array
			//  exec() is not suppose to return, something went wrong
			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
			fprintf(stderr, "%s :", _simpleCommands[i]->_arguments[0]);
			perror("");
			exit(2);
		}
		else
		{
			// Restore input, output, and error
			dup2(defaultin, 0);
			dup2(defaultout, 1);
			dup2(defaulterr, 2);
		}
		if (!_background)
			waitpid(pid, 0, 0);
	}

	// Close file descriptors that are not needed

	close(defaultin);
	close(defaultout);
	close(defaulterr);
	// Clear to prepare for next command
	clear();
	// Print new prompt
	prompt();
}

// Shell implementation

void Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

void catchControl_C(int sig_number)
{
	signal(SIGINT, catchControl_C);
	printf(" type exit to treminate the minishell\n");
	Command::_currentCommand.prompt();
}

void writeTerm(int sig_number)
{
	signal(SIGINT, catchControl_C);
	time_t now = time(0);
	char *dt = ctime(&now);
	int file = open("LogFile", O_CREAT | O_APPEND | O_WRONLY, 0666);
	if (file < 0)
	{
		exit(2);
	}
	int defaultin = dup(1);
	dup2(file, 1);
	printf("Child terminated at: %s", dt);
	close(file);
	dup2(defaultin, 1);
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int yyparse(void);

int main()
{
	signal(SIGINT, catchControl_C);
	signal(SIGCHLD, writeTerm);
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}
