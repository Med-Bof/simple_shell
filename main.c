#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define DELIMITERS " \t\r\n\a"

void handle_error(const char *argv0, const char *command, const char *message);

/**
 * display_prompt - Displays the shell prompt.
 *
 * Return: no return
 */
void display_prompt(void)
{
	printf("$ ");
}

/**
 * read_input - Reads the user input from the command line.
 *
 * Return: The input string.
 */
char *read_input(void)
{
	char *input = NULL;
	size_t len = 0;
	ssize_t read;

	read = getline(&input, &len, stdin);
	if (read == -1)
	{
		free(input);
		if (feof(stdin)) /* Handle Ctrl+D */
			exit(0);
		else
		{
			perror("getline");
			exit(EXIT_FAILURE);
		}
	}

	return (input);
}

/**
 * parse_input - Splits the input into command and arguments.
 *
 * @input: The input string.
 * Return: Array of command and arguments.
 */
char **parse_input(char *input)
{
	int bufsize = 64, position = 0;
	char **tokens = malloc(bufsize * sizeof(char *));
	char *token;

	if (!tokens)
	{
		fprintf(stderr, "allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(input, DELIMITERS);
	while (token != NULL)
	{
		tokens[position] = token;
		position++;

		if (position >= bufsize)
		{
			bufsize += 64;
			tokens = realloc(tokens, bufsize * sizeof(char *));
			if (!tokens)
			{
				fprintf(stderr, "allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, DELIMITERS);
	}
	tokens[position] = NULL;
	return (tokens);
}

/**
 * handle_path - Searches for the command in the PATH.
 *
 * @command: The command to find.
 * Return: The full path of the command if found, otherwise NULL.
 */
char *handle_path(char *command)
{
	char *path = getenv("PATH");
	char *path_copy = strdup(path);
	char *directory;
	char full_path[1024];

	directory = strtok(path_copy, ":");
	while (directory != NULL)
	{
		snprintf(full_path, sizeof(full_path), "%s/%s", directory, command);
		if (access(full_path, X_OK) == 0)
		{
			free(path_copy);
			return (strdup(full_path));
		}
		directory = strtok(NULL, ":");
	}
	free(path_copy);
	return (NULL);
}

/**
 * execute_command - Executes the command entered by the user.
 *
 * @args: The array of command and arguments.
 * @argv: The name of the shell executable (argv[0]).
 * Return: no return
 */
void execute_command(char **args, const char *argv)
{
	pid_t pid;
	int status;
	char *command;

	if (args[0] == NULL)
		return; /* Empty command was entered */

	command = handle_path(args[0]);
	if (command == NULL)
	{
		handle_error(argv, args[0], "command not found");
		return;
	}

	pid = fork();
	if (pid == 0)
	{
		/* Child process */
		if (execve(command, args, NULL) == -1)
		{
			perror("execve");
		}
		exit(EXIT_FAILURE);
	}
	else if (pid < 0)
	{
		/* Error forking */
		perror("fork");
	}
	else
	{
		/* Parent process */
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	free(command);
}

/**
 * handle_error - Handles errors and prints appropriate messages.
 *
 * @argv0: The name of the shell executable (argv[0]).
 * @command: The command that caused the error.
 * @message: The error message to print.
 * Return: no return
 */
void handle_error(const char *argv0, const char *command, const char *message)
{
	fprintf(stderr, "%s: %s: %s\n", argv0, command, message);
}

/**
 * main - Core main function of the shell
 *
 * @argc: Number of arguments passed to the program
 * @argv: Array of arguments
 * Return: Always 0
 */
int main(int argc, char *argv[])
{
	char *input;
	char **args;

	(void)argc; /* Unused parameter */
	(void)argv; /* Unused parameter */

	while (1)
	{
		display_prompt();
		input = read_input();
		args = parse_input(input);
		execute_command(args, argv[0]);
		free(input);
		free(args);
	}

	return (0);
}
