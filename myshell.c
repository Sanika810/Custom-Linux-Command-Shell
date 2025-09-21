#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

char currentDirectory[1024];
#define TOKEN_SIZE 128
#define COMMAND_SIZE 64

void parseInput(char *input_line , char **cmdarray , int *num_commands , const char *delimiter)
{
    int idx = 0;
    char *inputcopy = strdup(input_line);
    char *ptr = inputcopy;
    char *token;
    
    while((token = strsep(&ptr, delimiter)) != NULL) // extracting tokens separated by delimiter
    {
        while(*token == ' ' || *token == '\t') // skipping the leading tabs and spaces
        {
            token++; 
        }
        
        if(strlen(token) != 0)
        {
            cmdarray[idx] = strdup(token);
            idx++;
        }
           
    }
    
    cmdarray[idx] = NULL;//null termination
    *num_commands = idx;//storing size of command array
    free(inputcopy);
}

void executeCommand(char **tokens)
{
    if(tokens[0] != NULL && strlen(tokens[0]) != 0) //check for empty token
    {
        if(strcmp(tokens[0] , "exit") == 0)///handling exit
        {
            printf("Exiting shell...\n");
            exit(0);
        }

        if(strcmp(tokens[0] , "cd") == 0)//handling cd 
        {
            if(tokens[1] == NULL)
            {
                printf("Shell: Incorrect command\n");
                return;
            }
            char *path = tokens[1];
            if(chdir(path) != 0) //invalid path
            {
                printf("Shell: Incorrect command\n");
            }
            return;
        }
        
        pid_t pid = fork();
        if(pid < 0)
        {
            printf("Shell: Incorrect command\n");
            return;
        }
        if(pid == 0)
        {
            execvp(tokens[0] , tokens);
            printf("Shell: Incorrect command\n");
            exit(1);
        }
        else
        {
            waitpid(pid, NULL, 0);
        }
    }  
}


void executeParallelCommands(char *line)
{
    char *cmdarray[COMMAND_SIZE];
    int numcommands = 0;
    parseInput(line , cmdarray , &numcommands, "&&");
    pid_t pids[numcommands];

    for(int i = 0 ; i < numcommands ; i++)
    {
        char *tokens[TOKEN_SIZE];
        int numtokens = 0;
        char *inputcopy = strdup(cmdarray[i]);
        char *token;
        while((token = strsep(&inputcopy , " \t")) != NULL)
        {
            if(strlen(token) != 0)
            {
               tokens[numtokens] = token;
                numtokens++;
            }
                     
        }
        
        tokens[numtokens] = NULL;
        
        pid_t pid = fork();
        if(pid == 0)
        {
            executeCommand(tokens);
            exit(0);
        }
        
        else
        {
            pids[i] = pid;
        }
        free(cmdarray[i]);
        free(inputcopy);
    }
    for(int i = 0 ; i < numcommands ; i++)
    {
        waitpid(pids[i], NULL, 0);
    }
}

void executeSequentialCommands(char *line)
{
    char *cmdarray[COMMAND_SIZE];
    int numcommands = 0;
    parseInput(line , cmdarray , &numcommands , "##");

    for(int i = 0 ; i < numcommands ; i++)
    {
        char *tokens[TOKEN_SIZE];
        int numtokens = 0;
        char *inputcopy = strdup(cmdarray[i]);
        char *token;
        while((token = strsep(&inputcopy, " \t")) != NULL)
        {
            if(strlen(token) != 0)
            {
               tokens[numtokens] = token;
                numtokens++;
            }
        }
        tokens[numtokens] = NULL;
        executeCommand(tokens);
        free(cmdarray[i]);
        free(inputcopy);
    }
}

void executeCommandRedirection(char *line)
{
    char *cmdarray[2];
    int numcommands = 0;
    parseInput(line, cmdarray, &numcommands, ">");
    if(numcommands != 2)
    {
        printf("Shell: Incorrect command\n");
        return;
    }
    
    char *tokens[128];
    int numtokens = 0;
    char *inputcopy = strdup(cmdarray[0]);
    char *token;
    while((token = strsep(&inputcopy, " \t")) != NULL)
    {
        if(strlen(token) != 0)
        {
           tokens[numtokens] = token;
            numtokens++;
        }
    }
    tokens[numtokens] = NULL;

    char *outfile = cmdarray[1];
    while(*outfile == ' ' || *outfile == '\t')
    {
        outfile++;
    }
    
    if(*outfile == '\0')//empty file name
    {
        printf("Shell: Incorrect command\n");
        free(inputcopy);
        free(cmdarray[0]);
        free(cmdarray[1]);
        return;
    }

    pid_t pid = fork();
    if(pid < 0)
    {
        printf("Shell: Incorrect command\n");
        free(inputcopy);
        free(cmdarray[0]);
        free(cmdarray[1]);
        return;
    }
    if(pid == 0)
    {
        int fd_out = open(outfile , O_WRONLY | O_CREAT | O_TRUNC , 0644);
        if(fd_out < 0)
        {
            printf("Shell: Incorrect command\n");
            exit(1);
        }
        dup2(fd_out , STDOUT_FILENO);
        close(fd_out);
        execvp(tokens[0] , tokens);
        printf("Shell: Incorrect command\n");
        exit(1);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
    free(inputcopy);
    free(cmdarray[0]);
    free(cmdarray[1]);
}


void runPipeline(char *line)
{
    
    char *commandParts[COMMAND_SIZE];
    int howManyParts = 0;
    parseInput(line, commandParts, &howManyParts, "|");

    if(howManyParts < 2) 
    {
        fprintf(stderr, "Shell: Pipeline needs at least two commands\n");
        return;
    }

  
    int allPipes[2 * (howManyParts - 1)];
    for(int p = 0 ; p < howManyParts - 1 ; p++) 
    {
        if(pipe(allPipes + p * 2) < 0) 
        {
            perror("pipe creation failed");
            return;
        }
    }

    pid_t Workers[howManyParts];

    for(int step = 0 ; step < howManyParts ; step++) 
    {
        // chop each part into tokens (words) for execvp
        char *wordBox[TOKEN_SIZE];
        int wordCount = 0;
        char *lineCopy = strdup(commandParts[step]);
        char *chopped;
        while((chopped = strsep(&lineCopy , " \t")) != NULL) 
        {
            if(strlen(chopped) > 0) 
            {
                wordBox[wordCount] = chopped;
                wordCount++;
            }
        }
        wordBox[wordCount] = NULL;

        pid_t kid = fork();
        if(kid == 0) 
        {
            if(step > 0) 
            {
                dup2(allPipes[(step - 1) * 2] , STDIN_FILENO);
            }
            if(step < howManyParts - 1) 
            {
                dup2(allPipes[step * 2 + 1] , STDOUT_FILENO);
            }

            for(int z = 0 ; z < 2 * (howManyParts - 1) ; z++) 
            {
                close(allPipes[z]);
            }

            execvp(wordBox[0] , wordBox);
            fprintf(stderr , "Shell: couldn’t run command '%s'\n" , wordBox[0]);
            exit(1);
        } 
        else if(kid < 0) 
        {
            perror("fork failed in pipeline");
            for(int z = 0; z < 2 * (howManyParts - 1); z++) 
            {
                close(allPipes[z]);
            }
            return;
        } 
        else 
        {
            Workers[step] = kid;
        }

        free(commandParts[step]);
        free(lineCopy);
    }

    for(int z = 0 ; z < 2 * (howManyParts - 1) ; z++) 
    {
        close(allPipes[z]);
    }

    for(int step = 0 ; step < howManyParts ; step++) 
    {
        waitpid(Workers[step], NULL, 0);
    }
}


void handle_sigint(int sig)
{
    printf("\n");
    char dir[COMMAND_SIZE];
    if (getcwd(dir, sizeof(dir))) printf("%s$", dir);
    fflush(stdout);
}

void handle_sigstp(int sig)
{
    printf("\n");
    char dir[COMMAND_SIZE];
    if (getcwd(dir, sizeof(dir))) printf("%s$", dir);
    fflush(stdout);
}

int main()
{
    char *inputBuffer = NULL;
    size_t len = 0;
    ssize_t readStatus;

    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigstp);

    while (1)
    {
        if(getcwd(currentDirectory , sizeof(currentDirectory)) != NULL)
        {
            printf("%s$" , currentDirectory);

            readStatus = getline(&inputBuffer , &len , stdin);
            if(readStatus == -1)
            {
                break;
            }
            inputBuffer[strcspn(inputBuffer , "\n")] = 0;

            // Trim leading whitespace
            char *commandline = inputBuffer;
            while(*commandline == ' ' || *commandline == '\t')
            {
                commandline++;
            }
            
            if(strlen(commandline) == 0)
            {
                continue;
            }

            if (strcmp(commandline , "exit") == 0)
            {
                printf("Exiting shell...\n");
                break;
            }
            if(strstr(commandline , "&&"))
            {
                executeParallelCommands(commandline);
            }
            
            else if(strstr(commandline , "##"))
            {
                executeSequentialCommands(commandline);
            }
            
            else if(strstr(commandline , ">"))
            {
                executeCommandRedirection(commandline);
            }
            else if (strstr(commandline, "|"))
            {
                runPipeline(commandline);
            }

            else
            {
                char *tokens[TOKEN_SIZE];
                int numtokens = 0;
                char *inputcopy = strdup(commandline);
                char *token;
                while((token = strsep(&inputcopy, " \t")) != NULL)
                {
                    if(strlen(token) != 0)
                    {
                       tokens[numtokens] = token;
                        numtokens++;
                    }
                }
                tokens[numtokens] = NULL;
                
                if(numtokens == 0)
                {
                    printf("Shell: Incorrect command\n");
                }
                else
                {
                    executeCommand(tokens);
                }
                free(inputcopy);
            }               
        }               
    }
    free(inputBuffer);
    return 0;
}
