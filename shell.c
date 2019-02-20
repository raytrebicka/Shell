#include <stdio.h>
#include <stdlib.h>
#include <signal.h> // This is new!
#include <unistd.h> 
#include <string.h>
#include <sys/types.h>
#include <time.h>

// Global variable for max buffer size (characters that user can input to shell)
const int BUFFER_SIZE = 80;

// Signal handler for sig-int. When user presses ctrl-c "mini-shell terminated" will print and the mini-shell will be exited
void sigint_handler(int sig){
	write(1, "\nmini-shell terminated\n", 23);
	//free(commandList);
        //free(userTokens);
        //free(command1);
        //free(command2);
	exit(0);
}

// parseInput takes in a char* (string) and char** (userTokens) and parses the string to extract all its tokens, which are stored in userTokens
void parseInput(char* string, char** userTokens){
	
	char* token; // Pointer to char that will temporarily hold each token found in the input string
	int tokenPos = 0; // The index to keep track oof where the token is being stored in the userTokens array

	token = strtok(string," \t\r\n\a"); // First token in the string is found based on the given delimeters
	// Loops while tokens are still found in the input string
	while (token != NULL){
		userTokens[tokenPos] = token; // The token is stored in the userTokens array
		++tokenPos; // The index where the token is stored is incremented by 1
		token = strtok(NULL, " \t\r\n\a"); // The next token is found. Input is NULL because strtok sets a static pointer to the next token in the previous string
	}
	userTokens[tokenPos] = '\0'; // After all the tokens are found, the tokens array is ended with a null terminator

	free(token); // Frees the allocated token
}

// execute_without_pipe executes a command using execvp when there is not a pipe command input
void execute_without_pipe(char** command1) {

	pid_t pid; // Process ID that will result from the fork command to start a thread with parent and child processes
	int status; // Status integer that is used in the parent process to wait for the child process
	
	pid = fork(); // A new thread is spawned and the process ID is stored in pid
	if (pid < 0){ // For failed
		perror("ERROR: fork failed");
	}
	else if (pid == 0){ // Child process
		// In the child process, the comand is executed using execvp, and if it fails (command does not exist) and returns -1
		// a command not found prompt is returned
		if (execvp(*command1, command1) < 0){
			printf("mini-shell>Command not found--Did you mean something else?\n");	
			exit(1);
		}
	} else{ // Parent process (pid = 1)
		while (wait(&status) != pid); // The parent process waits for the child process to finish
	}
}

 // execute_with_pipe executes a command using piping with execvp when there is a pipe command input. The output of the first command is sent to the input of the second
void execute_with_pipe(char** command1, char** command2) {

	pid_t pid1, pid2; // Process ID's that will hold result from the fork commands to start threads with parent and child processes
	int status; // Status integer that is used in the parent process to wait for the child process

	int fd[2]; // File descriptor used for pipe
	pipe(fd); // Open pipe with indput fd

	pid1 = fork(); // A new process is spawned for the first command
	if (pid1 < 0) {
		perror("ERROR: fork failed");	
	} else if (pid1 == 0){ // Child process
		dup2(fd[1], 1); // stdout is hooked to the write end of the pipe
		close(fd[0]); // The read end of the pipe is no longer needed and is closed
		// The first command is executed
		if (execvp(command1[0], command1) < 0) {
			printf("mini-shell>Command not found--Did you mean something else?\n");
			exit(1);
		}
	}
 
	pid2 = fork(); // A new process is spawned for the second command
	if (pid2 < 0) {
                perror("ERROR: fork failed");
        } else if (pid2 == 0){ // Child process
		dup2(fd[0], 0); // stdin is hooked to the read end of the pipe
		close(fd[1]); // The write end of the pipe is no longer needed and is closed
		// The second command is executed
		if (execvp(command2[0], command2) < 0) {
			printf("mini-shell>Command not found--Did you mean something else?\n");
			exit(1);
		}
	}
	
	// Closes both the read and write ends of the pipe
	close(fd[0]);
	close(fd[1]);

	// Waits for everything to finish then exits
	waitpid(pid1);
	waitpid(pid2);								
}

// cd is a built-in function that takes in a char* input and changes the directory to the argument given
void cd(char* arg)
{
	// Checks if the input is null, and if so the arg is set to the home directory
	if (arg == '\0') {
		arg = "/home/raytrebicka";	
	}
	// chdir changes the directory to the agument. If it fails, a failure prompt is output.	
	if (chdir(arg) != 0) {
		perror("Unable to change into directory");
	} 
}

// help is a built-in function that prints all the built-in shell functions and their uses
void help(){
	printf("\nThe following built-in commands are provided:\n");
	printf("cd - Change directory (provide filepath as argument)\n");
	printf("help - List built-in shell commands\n");
	printf("shell_exit - Exit the shell\n");
	printf("guessinggame - Play a random number guessing game with the shell.\n\n");
}

// gusessinggame is a guessing game that the shell plays with the user.
void guessinggame() {
	printf("\nWelcome to the guessing game!\n");
	printf("The goal is to guess the random number that is generated from 1 to a number of your choice.\n");
	printf("Please input the biggest number to guess from: ");
	
	int max; // Will hold the max number to guess from (input by user)
	scanf("%d", &max); // Take in max number to guess from

	srand(time(NULL)); // Sets the seed of the random number generator
	int randNumber = rand() % max + 1; // random number is generated and moded by the max guess
	
	int guess; // Holds user guess
	int guessCounter = 0; // Counter for number of user guesses	
	int continueGame = 1; // Determines whether game should be continued
	while (continueGame) { // continueGame can be changed by the user after every wrong guess to exit the game
		printf("\nTake a guess: ");
		scanf("%d", &guess); // take in user guess
		++guessCounter; // Increment guess counter
		if (guess == randNumber) { // If the guess is correct, the while loop breaks
			printf("Correct! The number was %d.\n", randNumber);
			printf("You found it in %d guesses.\n", guessCounter);
			return;
		}
		if (guess > randNumber) { // If the guess is too high, prints a too high prompt
			printf("The guess was too high. Try guessing lower.\n");
		} 
		if (guess < randNumber) { // If the guess is too low, prints a too low prompt
                        printf("The guess was too low. Try guessing higher.\n");
                }
	
		printf("Would you like to guess again (0 - no, otherwise - yes)? ");
		scanf("%d", &continueGame); // Takes in input to decide if user wants to continue playing. If the user enters 0, the game ends, else the game continues	
	}
}

int main(){
	
	signal(SIGINT, sigint_handler); // Installs signal handler

	char* commandList[] = {"cd", "help", "exit", "guessinggame"}; // Stores the list of commands
	char string[BUFFER_SIZE]; // WIll store the user input string (initialized with 80 elements)
	
	char* userTokens[80]; // WIll stores the tokens in the user input
	userTokens[0] = NULL;
	char* command1[80]; // Will store the tokens in the first part of the user input (before the pipe | symbol)
	command1[0] = NULL;
	char* command2[80]; // Will store the tokens in the second part of the user input (after the pipe | symbol)	
	command2[0] = NULL;

	int userTokensIndex = 0; // Index tracker for looping through the userTokens
        int command2Index = 0; // Index tracker for looping through command after the pipe

	int counter1 = 0; // Index tracker for looping through command1 to clear the values
	int counter2 = 0; // Index tracker for looping through command2 to clear the values
	int counterTokens = 0; // Index tracker for looping through userTokens to clear the values
	
	while(1){
		printf("mini-shell>");
		
		// fgets takes in a string from the user and stores it in string. The max buffer size is 80
		fgets(string, BUFFER_SIZE, stdin);
		
		// isspace chaecks if the input string is just spaces, and if it is skips all the next commands and returns to the beginning of the while loop
		if (isspace(string[1])) {
			;
		} else if (string[1] != '\0' && string[1] != ' ' && string[1] != '\t') { // If the string is not null, the pprogram continues, otherwise nothing happens
			parseInput(string, userTokens);
		
			userTokensIndex = 0; // The userTokensIndex is set to 0
			command2Index = 0; // The command2Index is set to 0
			// userTokens is looped through and the commands and options before the '|' symbol are put in command1, and everything after the '|'
			// symbol is placed in command2 (which remains empty if there is no pipe)
			for(userTokensIndex = 0; userTokens[userTokensIndex] != '\0'; userTokensIndex++){ // Runs while usertokens is not null
				if(*userTokens[userTokensIndex] != 124){ // Compares the value of the current token to the ascii value for '|' 
					// If the current token is not after the pipe symbol, the current index of commmand1 is set to the current token
					command1[userTokensIndex] = userTokens[userTokensIndex];
				}
				else{ // Executed if the token is after the pipe symbol
					userTokensIndex++; // Index for the user tokens is incremented to go after the '|' symbol
					for(command2Index=0; userTokens[userTokensIndex] != '\0'; command2Index++){ // Runs while usertokens is not null
						command2[command2Index] = userTokens[userTokensIndex]; // The current index of commmand2 is set to the current token
						userTokensIndex++; // The userTokens index is incremented by 1
					}
					break; // Once the NULL of user tokens is reached, the outer for loop exits
				}
			}
			
			if (!strcmp(commandList[0], command1[0])) {
				cd(command1[1]); // Calls cd if the first token in command1 is "cd"
	                } else if(!strcmp(commandList[1], command1[0])) {
        	                help(); // Calls help() if the first token in command1 is "help"
                	} else if(!strcmp(commandList[2], command1[0])) {
				//free(commandList);
			        //free(userTokens);
			        //free(command1);
			        //free(command2);
				exit(0);
	                } else if(!strcmp(commandList[3], command1[0])) {
				guessinggame(); // Calls guessinggame if the first token in command1 is "guessinggame"
			} else if(command2[0] != NULL){ // If none of the previous conditions hold and the second command is not NULL (there was a pipe symbol)
				execute_with_pipe(command1, command2); // execute_with_pipe is called with argumens of commmand1 and command2 are given
			} else { // If none of the built-ins are called and command two is null, this is entered
				execute_without_pipe(userTokens); // execute_without_pipe is called with command1 given
			}
			
			// The counter for command1 is set to 0. command1 is looped through while it is not null and each
			// non-null element is set to null
			counter1 = 0;
                       	while(command1[counter1] != '\0') {
                        	command1[counter1] = '\0';
                                counter1++;
               		}   	
			
			// The counter for command2 is set to 0. command2 is looped through while it is not null and each
			// non-null element is set to null
                        counter2 = 0;
                        while(command2[counter2] != '\0') {
     		        	command2[counter2] = '\0';
                                counter2++;
			}
			
			// The counter for command2 is set to 0. command2 is looped through while it is not null and each
			// non-null element is set to null
			counterTokens = 0;
                        while(userTokens[counterTokens] != '\0') {
                                userTokens[counterTokens] = '\0'; 
                                counterTokens++;
                        }
		}
	}
	
	return 0;	
}

