
#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()

typedef struct {
    char** commandTokens;
    int val;       
} cmd;

cmd parseInput(char *in)
{
	// This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter (&&, ##, >, or spaces).
	cmd cmd1;
	cmd1.commandTokens = malloc(8 * (sizeof(char *)));
	cmd1.val=0;
	int index = 0;
	char *delimiter = " \t\r\n";
	char *token;
	
	token = strtok(in, delimiter);
	while(token != NULL)
	{
		size_t len = strlen(token);
        if (token[len - 1] == '\n') 
		{
            token[len - 1] = '\0'; //removing newline character
        }
		cmd1.commandTokens[index] = token;
		index++;
		
		token = strtok(NULL, delimiter);
	}
	cmd1.commandTokens[index]=NULL;
	
	size_t len = strlen(cmd1.commandTokens[0])-1 ;
	index = 0;
	
	if(*cmd1.commandTokens[index] && cmd1.commandTokens[index][len] == '\n')
	{
		cmd1.commandTokens[index][len] = '\0';
	}
	
	if(strcmp(cmd1.commandTokens[index], "exit") == 0)
	{
		cmd1.val=1;
		return cmd1;
	}
	
	while(cmd1.commandTokens[index])
	{
		if(strcmp(cmd1.commandTokens[index], "&&") == 0)
		{
			cmd1.val=2;
			return cmd1;	
		}
		index++;
	}
	
	index = 0;
	while(cmd1.commandTokens[index])
	{
		if(strcmp(cmd1.commandTokens[index], "##") == 0)
		{
			cmd1.val=3;
			return cmd1;	
		}
		index++;
	}
	
	index = 0;
	while(cmd1.commandTokens[index])
	{
		if(strcmp(cmd1.commandTokens[index], ">") == 0)
		{
			cmd1.val=4;
		        return cmd1;	
		}
		index++;
	}
	return cmd1;
}


void executeCommand(char **commandTokens)
{
	// This function will fork a new process to execute a command
	if(strcmp(commandTokens[0], "cd") == 0)
	{
		if( chdir(commandTokens[1]) < 0 )
		{
			printf("Shell: Incorrect command\n");	
		}
		return;
	}
	else
	{
		int rc = fork();
		
        if(rc < 0)
        {
            printf("There's an error while forking");
        }
        
        if (rc == 0)
        {
            if (execvp(commandTokens[0], commandTokens) == -1)
            {
                printf("Shell: Incorrect command\n");
            }
            exit(0);
        }
        
        else
        {
            wait(0);
        }
    }
}

void executeParallelCommands(char **commandTokens)
{
	// This function will run multiple commands in parallel
	pid_t pid;
	int status;
	int index = 0;
	while (commandTokens[index] != NULL) {
        char *args[100];
        int j = 0;

        while (commandTokens[index] != NULL && strcmp(commandTokens[index], "&&") != 0) {
            args[j] = commandTokens[index];
            index++;
            j++;
        }

        args[j] = NULL;

        if (args[0] != NULL) {
            // Execute the command in the background
            executeCommand(args);
        }

        if (commandTokens[index] != NULL && strcmp(commandTokens[index], "&&") == 0) {
            index++;
        }
    }

    // Wait for all background processes to finish
    while ((pid = wait(&status)) > 0);
}

void executeSequentialCommands(char **commandTokens)
{	
	// This function will run multiple commands in parallel
    int index = 0;

    while (commandTokens[index] != NULL) {
        char *args[100];
        int j = 0;

        while (commandTokens[index] != NULL && strcmp(commandTokens[index], "##") != 0) {
            args[j] = commandTokens[index];
            index++;
            j++;
        }

        args[j] = NULL;

        if (args[0] != NULL) {
            executeCommand(args); // Execute the command sequentially
        }

        if (commandTokens[index] != NULL && strcmp(commandTokens[index], "##") == 0) {
            index++;
        }
    }
}

void executeCommandRedirection(char **commandTokens)
{
	// This function will run a single command with output redirected to an output file specificed by user
    char *outputFile = NULL;
    int index = 0;

    while (commandTokens[index] != NULL) {
        if (strcmp(commandTokens[index], ">") == 0) {
            outputFile = commandTokens[index + 1];
            break;
        }
        index++;
    }

    if (outputFile == NULL) {
        printf("Shell: Incorrect command\n");

        return;
    }

    int outputFD = open(outputFile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IXUSR);

    if (outputFD == -1) {
        printf("Shell: Incorrect command\n");

        return;
    }

    int output = dup(1);
    dup2(outputFD, 1);
    close(outputFD); // Close the file descriptor after redirection

    char *commandArgs[index + 1];
    for (int i = 0; i < index; i++) {
        commandArgs[i] = commandTokens[i];
    }
    commandArgs[index] = NULL;

    executeCommand(commandArgs); // Execute the custom command

    dup2(output, 1); // Restore the original standard output
}


int main()
{
	// Initial declarations
	char **commands;
	
	while(1)	// This loop will keep your shell running until user exits.
	{
		// Print the prompt in format - currentWorkingDirectory$
		char cwd[1024];
    		getcwd(cwd, sizeof(cwd));
    		printf("%s$", cwd);
			
		// accept input with 'getline()'
		size_t buffer_size = 80;
        	char *input = malloc(buffer_size);
        	getline(&input, &buffer_size, stdin);

		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		cmd cmd1 = parseInput(input); 	
		char **commands = cmd1.commandTokens;
		int ret_val = cmd1.val;
			
		
		if(ret_val == 1)	// When user uses exit command.
		{
			printf("Exiting shell...\n");
			exit(0);		
		}
		
		if (ret_val == 2)
			executeParallelCommands(commands);		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
		
		else if (ret_val == 3)
			executeSequentialCommands(commands);	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
		
		else if (ret_val == 4)
			executeCommandRedirection(commands);	// This function is invoked when user wants redirect output of a single command to and output file specificed by user
		
		else
			executeCommand(commands);
			
				// This function is invoked when user wants to run a single commands			
	free(input);
	free(commands);}
	
	
	return 0;
}
