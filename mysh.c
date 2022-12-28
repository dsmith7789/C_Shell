// Shell for p1b

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX 512

/* STRUCTURES */
typedef struct aliasNode {
    char* aliasName;
    char* actualCommand[512];
    int aliasNumArgs;
    struct aliasNode* next;
} aliasNode;

/* PROTOTYPES */
void executeCommand(char* parsedCommand[], char* fileName);
int numberArguments(char* command);
char** processCommand(char* command);
int checkForRedirection(char* parsedCommand[], int numArgs);
int checkAliasCommandFormat(char* parsedCommand[], int numArgs);
void addAlias(char* aliasName, char** actualCommand, int aliasNumArgs);
void removeAlias(char* aliasName);
aliasNode* getAliasNode(char* aliasName);
void handleAliasing(int aliasType, char* aliasName, char** actualCommand, int aliasNumArgs);
void printAliasNode(aliasNode* current);


void interactive();
void batch(char* filename);


/* GLOBAL VARIABLES */
aliasNode* head;
int aliasListExists = 0;

/* Given an alias name, retrieve the entire alias node */
aliasNode* getAliasNode(char* aliasName) {
	if (aliasListExists == 1) {
		aliasNode* current = head;
		while (current != NULL) {
			if (strcmp(current->aliasName, aliasName) == 0) {
				return current;
			}
			current = current->next;
		}
		// if we got here, we went through the whole list w/out finding the alias, so it's not there
		return NULL;
	} else {
		return NULL;
	}
}

/* Adds an alias to the linked list */
void addAlias(char* aliasName, char** actualCommand, int aliasNumArgs) {
	if (aliasListExists == 1) {
		// if the alias was already there, then remove it so we can replace it
		if (getAliasNode(aliasName) != NULL) {			
			write(1, aliasName, strlen(aliasName));
			for (int i = 0; i < aliasNumArgs; i++) {
				write(1, " ", 1);
				write(1, actualCommand[i], strlen(actualCommand[i]));
			}
			write(1, "\n", 1);
			removeAlias(aliasName);
		}
		aliasNode* current = head;
		// traverse to the end of the linked list
		while (current->next != NULL) {
			current = current->next;
		}
		aliasNode* newNode = (aliasNode*) malloc(sizeof(aliasNode));
		if (newNode == NULL) {
			char* failString = "malloc failed to acquire pointer for newNode.\n";
			write(1, failString, strlen(failString));
			exit(1);
		}
		newNode->aliasName = aliasName;
        for (int i = 0; i < aliasNumArgs; i++) {
            newNode->actualCommand[i] = actualCommand[i];
        }
		//newNode->actualCommand = actualCommand;
		newNode->aliasNumArgs = aliasNumArgs;
		newNode->next = NULL;
		current->next = newNode;
	} else {
		head = (aliasNode*) malloc(sizeof(aliasNode));	// head is declared but doesn't really exists until now
		if (head == NULL) {
			char* failString = "malloc failed to acquire pointer for head.\n";
			write(1, failString, strlen(failString));
			exit(1);
		}
		head->aliasName = aliasName;
        for (int i = 0; i < aliasNumArgs; i++) {
            head->actualCommand[i] = actualCommand[i];
        }
		//head->actualCommand = actualCommand;
		head->aliasNumArgs = aliasNumArgs;
		head->next = NULL;
		aliasListExists = 1;
	}
}

/* Given an alias name, remove the alias from the linked list. Returns the removed actual command. */
void removeAlias(char* aliasName) {
	if (aliasListExists == 1) {
		aliasNode* current = head;
		aliasNode* previous = head;
		//char* returnValue;
		
		while (current != NULL) {
			// if we found a match
			if (strcmp(current->aliasName, aliasName) == 0) {
				// and the matching node is the first in the list
				if (current == head) {
					// and there's only one node in the list
                    if (current->next == NULL) {
                        //returnValue = current->value;
                        free(head); // we used malloc to reserve head, now need to free it
                        aliasListExists = 0;   // the list doesn't exist anymore, will need to make new
                        return; //returnValue;
                    } else {
                        // but there's more than one node in the list
                        aliasNode* oldHead = head;  // so we can free the old pointer
                        head = current->next;   // remove the node from the list
                        free(oldHead);
                        return;
                    }
				} else {
					// and the matching node isn't the first in the list
					//returnValue = current->value;
                    previous->next = current->next;
                    free(current);
                    return;
				}
			}
			previous = current;
			current = current->next;
		}
	} else {
		// if we got here, we searched the whole list and didn't find the alias so it's not there
		return; // NULL;
	}
}

/* For an aliasNode, print the alias name and the actual command that the alias maps to */
void printAliasNode(aliasNode* current) {
	write(1, current->aliasName, strlen(current->aliasName));
	for (int i = 0; i < current->aliasNumArgs; i++) {
		write(1, " ", 1);
		write(1, current->actualCommand[i], strlen(current->actualCommand[i]));
	}
    write(1, "\n", 1);  // new line for each alias
}

/* contains all the code to handle user's alias/unalias request */
void handleAliasing(int aliasRequestType, char* aliasName, char** actualCommand, int aliasNumArgs) {
	// Trying to alias a prohibited word (alias, unalias, exit)
	if (aliasRequestType == 0) {
		char* dangerousAliasString = "Too dangerous to alias that.\n";
		write(STDERR_FILENO, dangerousAliasString, strlen(dangerousAliasString));
	}
	// Trying to make a new valid alias
	if (aliasRequestType == 1) {
		addAlias(aliasName, actualCommand, aliasNumArgs);
	}
	// Trying to list all aliases
	if (aliasRequestType == 2) {
		aliasNode* current = head;
		while (current != NULL) {
			printAliasNode(current);
            current = current->next;
		}
	}
	// Trying to print the actual command associated with the alias
	if (aliasRequestType == 3) {
		aliasNode* current = getAliasNode(aliasName);
		if (current != NULL) {
			printAliasNode(current);
		}
	} 
	// Trying to unalias something
	if (aliasRequestType == 4) {
		removeAlias(aliasName);
	}
}

/* This mode allows users to manually input commands to the shell */
void interactive() {
	while (1) {
		char* prompt = "(Dante Shell) > ";
		write(1, prompt, strlen(prompt));
		char buf[MAX];
        char* input = fgets(buf, MAX, stdin);
		input[strcspn(input, "\n")] = 0;    // parse the entry to remove trailing newline from fgets
		int exitRequested = strcmp(input, "exit");
		if (exitRequested == 0) {
			exit(1);
		}		
		char* command = calloc(1, MAX * sizeof(char));
		if (command == NULL) {
			char* failString = "calloc failed to acquire pointer for command.\n";
			write(1, failString, strlen(failString));
			exit(1);
		}
		strcpy(command, input);
		
		int numArgs = numberArguments(command);
		char* argumentVector[numArgs + 1];
		char** splitCommand;
		splitCommand = processCommand(command);
		for (int i = 0; i <= numArgs; i++) {
			if (i < numArgs) {
                //argumentVector[i] = splitCommand[i * sizeof(char*)];
                argumentVector[i] = splitCommand[i];
            }
            else {
                argumentVector[i] = NULL;
            }
		}
		
		// check if requesting alias
		char* aliasName = (char*) malloc(sizeof(char));
		if (aliasName == NULL) {
			char* failString = "malloc failed to acquire pointer for alias name.\n";
			write(1, failString, strlen(failString));
			exit(1);
		}
		
		char** actualCommand = (char**) malloc(sizeof(char*));
		if (actualCommand == NULL) {
			char* failString = "malloc failed to acquire pointer for alias actual comand.\n";
			write(1, failString, strlen(failString));
			exit(1);
		}
		aliasName = argumentVector[1];
		for (int i = 2; i < numArgs; i++) {
			actualCommand[i - 2] = argumentVector[i];
		}
		int aliasNumArgs = numArgs - 2;	
		int aliasRequestType = checkAliasCommandFormat(argumentVector, numArgs);
		
		// if we're actually requesting an alias, parse out the alias name and the actual command
		if (aliasRequestType != -1) {
			handleAliasing(aliasRequestType, aliasName, actualCommand, aliasNumArgs);
            continue;   // continue so we don't try to actually run the alias
		} else {
			// not requesting an alias so we don't need the heap pointers to actualCommand anymore
			free(actualCommand);
		}
		
		// if the user creating/listing out aliases, check if they're trying to execute a command via alias
		aliasNode* relevantAliasNode = getAliasNode(argumentVector[0]);
		
		// now that we have the command separated into an array, we need to check if there's any redirection
		int redirectionCheck = checkForRedirection(argumentVector, numArgs);
		if (redirectionCheck == 0) {
			if (relevantAliasNode == NULL) {
				executeCommand(argumentVector, NULL);
			}
			else {
				executeCommand(relevantAliasNode->actualCommand, NULL);
			}
		}
		if (redirectionCheck == 1) {
			int newNumArgs;
			newNumArgs = numArgs - 2; // 2 less arguments; take off the redir and the file
			char* newArgVector[numArgs - 2];
			for (int i = 0; i <= newNumArgs; i++) {  
				if (i < newNumArgs) {
					newArgVector[i] = argumentVector[i];
				}
				else {
					newArgVector[i] = NULL;
				}
			}
			char* fileName = argumentVector[numArgs - 1];
			executeCommand(newArgVector, fileName);
		}
		if (redirectionCheck == 2) {
			char* redirMisformat = "Redirection misformatted.\n";
			write(1, redirMisformat, strlen(redirMisformat));
		}

	}
}

/* This mode allows users to run commands from file */
void batch(char* filename) {
	FILE* commandFile = fopen(filename, "r");
	// handle gracefully if we're unable to open the file
	if (commandFile == NULL) {
		char* errorMessage = strcat("Cannot open file ", filename);
		int messageLength = strlen(errorMessage) + 1;
		write(STDERR_FILENO, strcat(errorMessage, "\n"), messageLength);
		exit(1);
	}
	char* buffer = (char*) calloc(1, MAX * sizeof(char));
    if (buffer == NULL) {
        printf("calloc not able to obtain pointer for buffer in batch mode.\n");
        exit(1);
    }
    size_t numCharRead;
    size_t bufSize = MAX;

    while ((numCharRead = getline(&buffer, &bufSize, commandFile)) != -1) {
        char* command = calloc(1, MAX * sizeof(char));
        if (command == NULL) {
            printf("calloc failed to obtain pointer for command.\n");
            exit(1);
        }
        strcpy(command, buffer);
		write(1, strcat(command, "\n"), strlen(command)); // echo back to user
		command[strcspn(command, "\n")] = 0;     // remove trailing new line from fgets
		int exitRequested = strcmp(command, "exit");
		if (exitRequested == 0) {
			exit(1);
		}
		
		int numArgs = numberArguments(command);
		char* argumentVector[numArgs + 1];
		char** splitCommand;
		splitCommand = processCommand(command);
		for (int i = 0; i <= numArgs; i++) {
			if (i < numArgs) {
                //argumentVector[i] = splitCommand[i * sizeof(char*)];
                argumentVector[i] = splitCommand[i];
            }
            else {
                argumentVector[i] = NULL;
            }
		}
		
		// check if requesting alias
		char* aliasName = (char*) malloc(sizeof(char));
		if (aliasName == NULL) {
			char* failString = "malloc failed to acquire pointer for alias name.\n";
			write(1, failString, strlen(failString));
			exit(1);
		}
		
		char** actualCommand = (char**) malloc(sizeof(char*));
		if (actualCommand == NULL) {
			char* failString = "malloc failed to acquire pointer for alias actual comand.\n";
			write(1, failString, strlen(failString));
			exit(1);
		}
		aliasName = argumentVector[1];
		for (int i = 2; i < numArgs; i++) {
			actualCommand[i - 2] = argumentVector[i];
		}
		int aliasNumArgs = numArgs - 2;	
		int aliasRequestType = checkAliasCommandFormat(argumentVector, numArgs);
		
		// if we're actually requesting an alias, parse out the alias name and the actual command
		if (aliasRequestType != -1) {
			handleAliasing(aliasRequestType, aliasName, actualCommand, aliasNumArgs);
            continue;   // continue so we don't try to run the alias
		} else {
			// not requesting an alias so we don't need the heap pointers to actualCommand anymore
			free(actualCommand);
		}
		
		// if the user creating/listing out aliases, check if they're trying to execute a command via alias
		aliasNode* relevantAliasNode = getAliasNode(argumentVector[0]);
		
		// now that we have the command separated into an array, we need to check if there's any redirection
		int redirectionCheck = checkForRedirection(argumentVector, numArgs);
		if (redirectionCheck == 0) {
			if (relevantAliasNode == NULL) {
				executeCommand(argumentVector, NULL);
			}
			else {
				executeCommand(relevantAliasNode->actualCommand, NULL);
			}
		}
		if (redirectionCheck == 1) {
			int newNumArgs;
			newNumArgs = numArgs - 2; // 2 less arguments; take off the redir and the file
			char* newArgVector[numArgs - 2];
			for (int i = 0; i <= newNumArgs; i++) {  
				if (i < newNumArgs) {
					newArgVector[i] = argumentVector[i];
				}
				else {
					newArgVector[i] = NULL;
				}
			}
			char* fileName = argumentVector[numArgs - 1];
			executeCommand(newArgVector, fileName);
		}
		if (redirectionCheck == 2) {
			char* redirMisformat = "Redirection misformatted.\n";
			write(1, redirMisformat, strlen(redirMisformat));
		}
	}
}

/* MAIN METHOD */
int main(int argc, char* argv[]) {   
	if (argc == 2) {
		// Batch mode
		batch(argv[1]);	
	} else if (argc == 1) {
		// Interactive mode
		interactive();
	} else {
		// Not using mysh correctly
		char* usageStatement = "Usage: mysh [batch-file]\n";
		write(1, usageStatement, strlen(usageStatement));
		exit(1);
	}
	return 0;
}


/* Classifies the type of alias request. Returns:
	-1: if the user didn't request an alias or an unalias
    0: if the user is trying to alias something too dangerous (ex. alias, unalias, exit)
    1: if the user is trying to create a new alias (ex. alias ll /bin/ls -l -a)
    2: if the user is trying to list all the aliases created (ex. just typed "alias")
    3: if the user is trying to check the actual command in the alias (ex. alias ll returns /bin/ls -l -a)
	4: if the user is trying to remove an alias (aka "unalias")
*/
int checkAliasCommandFormat(char* parsedCommand[], int numArgs) {
	
	int aliasRequested = 0;
	int unaliasRequested = 0;
	
	// did the user actually request an alias?
	if (strcmp(parsedCommand[0], "alias") == 0) {
		aliasRequested = 1;
	}
	if (strcmp(parsedCommand[0], "unalias") == 0) {
		unaliasRequested = 1;
	}
	// we're not requesting an alias
	if (aliasRequested == 0) {
		// and not requesting an unalias, so user is not trying to alias
		if (unaliasRequested == 1) {
			return 4;
		} else {
			// but we are requesting an unalias
			return -1;
		}
	}

    // trying to list all aliases
    if (numArgs == 1) {
        return 2;
    }
    // trying to see the actual command behind the alias
    if (numArgs == 2) {
        return 3;
    }

    int aliasCheck = strcmp(parsedCommand[1], "alias");
    int unaliasCheck = strcmp(parsedCommand[1], "unalias");
    int exitCheck = strcmp(parsedCommand[1], "exit");

    // trying to alias a prohibited word
    if ((aliasCheck == 0) || (unaliasCheck == 0) || (exitCheck == 0)) {
        return 0;
    }
    
    // making a new alias
    return 1;
}


/* Given an array of arguments, check if we're using the redirection ">" operator. Returns:
    0: if there's no attempt at redirection in the command
    1: if there is redirection and it's formatted correctly
    2: if there was some attempt at redirection but it wasn't formatted correctly.
*/
int checkForRedirection(char* parsedCommand[], int numArgs) {
    int foundRedirector = 0;    // "boolean" to track if we found the redirector
    int redirectorIndex = 0;    // tracks which argument contained the redirector

    // search for the redirector operator
    for (int i = 0; i < numArgs; i++) {
        if (*parsedCommand[i] == '>') {
            foundRedirector = 1;
            redirectorIndex = i;
        }
    }

    // if we just never found the redirector, then the user is not trying redirection
    if (foundRedirector == 0) {
        return 0;
    }
    
    // the redirector can't be the first argument, this is a formatting error, to be handled later
    if (redirectorIndex == 0) {
        return 2;
    }

    // if there's not exactly one thing after the redirector, this is a formatting error, to be handled later
    if (redirectorIndex == (numArgs - 1)) {
        return 2;
    }
    
    // if we have two redirectors in a row, this is a formatting error, to be handled later
    if (*parsedCommand[redirectorIndex + 1] == '>') {
        return 2;
    }

    // if there's multiple files listed after the redirector, this is a formatting error, to be handled later
    if (redirectorIndex != (numArgs - 2)) {
        return 2;
    }

    // otherwise, we have a "valid" redirection (this is yet to be determined, but the formatting is correct
    return 1;    
}


/* Given an array of arguments, execute the command */
void executeCommand(char* parsedCommand[], char* fileName) {

    // CASE: No redirection
    if (fileName == NULL) {

        pid_t child_pid;
    
        child_pid = fork();
    
        // child process
        if (child_pid == 0) {
            execv(parsedCommand[0], parsedCommand);
            // if we got past that line, execv returned, meaning that the command failed
            char* errorMessage = strcat(parsedCommand[0], ": Command not found.\n");
            int messageLength = strlen(errorMessage);
            write(STDERR_FILENO, errorMessage, messageLength);
            _exit(0);
        }
        // parent process. Waits for child to finish.
        else {
            waitpid(child_pid, NULL, 0);
        }
    }
    // CASE: Redirection
    else {
        pid_t child_pid;
    
        child_pid = fork();
    
        // child process
        if (child_pid == 0) {
            
            int file = open(fileName, O_CREAT | O_TRUNC | O_RDWR, 0777);

            // if couldn't open the file
            if (file == -1) {
                char* fileOpenErrMsg = strcat("Cannot write to file ", fileName);
                int fileOpenErrMsgLen = strlen(fileOpenErrMsg) + 2;
                write(STDERR_FILENO, strcat(fileOpenErrMsg, ".\n"), fileOpenErrMsgLen);
                _exit(0);
            }

            dup2(file, STDOUT_FILENO);
            close(file);

            execv(parsedCommand[0], parsedCommand);

            // if we got past that line, execv returned, meaning that the command failed
            char* errorMessage = strcat(parsedCommand[0], ": Command not found.\n");
            int messageLength = strlen(errorMessage);
            write(STDERR_FILENO, errorMessage, messageLength);
            _exit(0);
        }
        // parent process. Waits for child to finish.
        else {
            waitpid(child_pid, NULL, 0);
        }

    }

}

/* Given a line of commands, count how many commands were given */
int numberArguments(char* command) {

    int count = 0;
    int previouslyAtWhiteSpace = 0;

    for (int i = 0; i <= strlen(command); i++) {
        if (!previouslyAtWhiteSpace && (command[i] == ' ' || command[i] == '\t' || command[i] == '\n' || command[i] == '\0')) {
            previouslyAtWhiteSpace = 1;
            count++;
        }
        else if ((command[i] != ' ') && (command[i] != '\t') && (command[i] != '\n') && (command[i] != '\0')) {
            previouslyAtWhiteSpace = 0;
        }
    }
    return count;
}

/* Given a line of commands, break this into an argument vector */
char** processCommand(char* command) {
    
    int i = 0;

    int previouslyAtWhiteSpace = 0;

    // first count all the arguments
    int count = numberArguments(command);
    
    // we can only take 99 different arguments max
    if (count > 99) {
        count = 99;
    }

    // reserve enough space for all the arguments plus the terminating null
    char** splitCommand = (char**) malloc((count + 1) * sizeof(char*)); 

    // now create a char* for each argument. Assuming max length of argument is 512
    for (i = 0; i < count; i++) {
        char* argumentPointer = malloc(512 * sizeof(char));
        splitCommand[i] = argumentPointer;

        //*(splitCommand + (i * sizeof(char*))) = argumentPointer;
        
        //char** location = splitCommand + (i * sizeof(char*));
        //*location = argumentPointer;
    }
    // now we move the arguments from the command into the splitCommand argument vector
    previouslyAtWhiteSpace = 0;
    int currentWordNumber = 0;
    int currentWordLetterIndex = 0;

    for (i = 0; (i <= strlen(command) + 1); i++) {
        // if we've already read in all the words in command (to prevent reading in random things)
        if (currentWordNumber == count) {
            break;
        }
        //char* location = *(splitCommand + (currentWordNumber * sizeof(char*)));
        char* location = splitCommand[currentWordNumber];

        // when ending a word, enter a null character to signify new entry in array
        if (!previouslyAtWhiteSpace && (command[i] == ' ' || command[i] == '\t' || command[i] == '\n' || command[i] == '\0')) {
            previouslyAtWhiteSpace = 1;
            *(location + currentWordLetterIndex) = '\0';
            currentWordNumber++;
            currentWordLetterIndex = 0;
        }
        // when we're at a word, copy the character from the command to the array
        else if ((command[i] != ' ') && (command[i] != '\t') && (command[i] != '\n') && (command[i] != '\0')) {
            previouslyAtWhiteSpace = 0;
            *(location + currentWordLetterIndex) = command[i];
            currentWordLetterIndex++;
        }
    }
    return splitCommand;
}





