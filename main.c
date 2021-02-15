// Harjasleen Gulati (ONID: gulatih)
// 2/8/2021
// OS 344: Operating Systems I 

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

int foregroundMode = 0;

// Used lecture notes as reference
void catchSIGTSTP(int signal) {
    if(foregroundMode == 0) {
        foregroundMode = 1;
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 70);
        fflush(stdout);

    } else {
        foregroundMode = 0;
        char* message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 70);
        fflush(stdout);

    }
}

// Used lecture notes as reference
void checkExitStatus(int status) {
    if(WIFEXITED(status)) {
        printf("exit value %i \n", WEXITSTATUS(status));
        fflush(stdout);

    } else {
        printf("terminated by signal %i \n", status);
        fflush(stdout);
    }
}

void changeDirectories(char **commandLineArguments) {
    if(commandLineArguments[1] == NULL) { // stay in the current directory
        chdir(getenv("HOME"));

    } else {
        chdir(commandLineArguments[1]);

    }
}

int main() {
    int backgroundProcess = 0;
    //char commandLineInput[2048]; // 2048 is max number of characters in command line input
    //commandLineInput[0] = '/0';
    char * commandLineInput = NULL;
    char *commandLineArguments[512]; // 512 is max number of arguments in command line input
    char *token = NULL;
    char *outputFilename = NULL;
    char *inputFilename = NULL;
    size_t len = 0;
    ssize_t nread;
    int status = 0;
    pid_t child_pid = -5;
    int childExitStatus = -5;

    // Used lecture notes as reference
    struct sigaction SIGINT_action = {0};
    struct sigaction SIGTSTP_action = {0};

    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;

    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;

    sigaction(SIGINT, &SIGINT_action, NULL);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    while(1) {
        backgroundProcess = 0;
        int arr_index = 0;

        // prompt for user input
        printf(": ");
        fflush(stdout);

        nread = getline(&commandLineInput, &len, stdin);

        token = strtok(commandLineInput, " \n");

        while(token != NULL) {
            // store filename for output redirection
            if(strcmp(token, ">") == 0) {
                token = strtok(NULL, " \n");
                outputFilename = calloc(strlen(token) + 1, sizeof(char));
                strcpy(outputFilename, token);
                token = strtok(NULL, " \n");

            // store filename for input redirection
            } else if(strcmp(token, "<") == 0) {
                token = strtok(NULL, " \n");
                inputFilename = calloc(strlen(token) + 1, sizeof(char));
                strcpy(inputFilename, token);
                token = strtok(NULL, " \n");

            // Activate background process
            } else if(strcmp(token, "&") == 0) {
                backgroundProcess = 1;
                break;

            // Expand PID
            } else if(strstr(token, "$$") != NULL) {

            // Since it is not a special UNIX symbol, that means that it is an argument
            } else {
                commandLineArguments[arr_index] = strdup(token);
                token = strtok(NULL, " \n");
                arr_index++;
            }
        }

        // If the user enters a comment or blank line
        if(*(commandLineArguments[0]) == '#' || commandLineArguments[0] == NULL) {
            continue;

        } else if(strcmp(commandLineArguments[0], "exit") == 0) {
            exit(0);

        } else if(strcmp(commandLineArguments[0], "status") == 0) {
            checkExitStatus(status);

        } else if(strcmp(commandLineArguments[0], "cd") == 0) {
            changeDirectories(commandLineArguments);

        } else { // not a built-in command
            child_pid = fork();

            if(child_pid == -1) {
                perror("Hull Breach!\n");
                exit(1);


            } else if(child_pid == 0) {
                if(!backgroundProcess) { // if it is a foreground process
                    SIGINT_action.sa_handler = SIG_DFL;
                    sigaction(SIGINT, &SIGINT_action, NULL);

                }

                if(outputFilename != NULL) { // output redirection has been specified by user
                    // Used lecture notes as reference
                    int targetFD = open(outputFilename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if(targetFD == -1) {
                        perror("open()");
                        printf("cannot open %s for output\n", outputFilename);
                        fflush(stdout);
                        exit(1);

                    }

                    int result = dup2(targetFD, 1);

                    if(result == -1) {
                        perror("dup2()");
                        exit(2);

                    }

                    close(targetFD);

                }

                if(inputFilename != NULL) { // input redirection has been specified by user
                // Used lecture notes as reference
                int targetFD = open(inputFilename, O_RDONLY);
                if(targetFD == -1) {
                    perror("open()");
                    printf("cannot open %s for input\n", inputFilename);
                    fflush(stdout);
                    exit(1);

                }

                int result = dup2(targetFD, 0);

                if(result == -1) {
                    perror("dup2()");
                    exit(2);

                }

                close(targetFD);

            }

            if(backgroundProcess == 1) {
                if(outputFilename == NULL) { // a background command and no output redirection specified by user
                    int targetFD = open("/dev/null", O_WRONLY);
                    if(targetFD == -1) {
                        perror("cannot open /dev/null for output");
                        printf("cannot open /dev/null for output\n");
                        fflush(stdout);
                        exit(1);

                    }

                    int result = dup2(targetFD, 1);

                    if(result == -1) {
                        perror("dup2");
                        exit(2);

                    }

                    close(targetFD);

                }

                if(inputFilename == NULL) { // a background command and no input redirection specified by user
                    int targetFD = open("/dev/null", O_RDONLY);
                    if(targetFD == -1) {
                        perror("cannot open /dev/null for output");
                        printf("cannot open /dev/null for output \n");
                        fflush(stdout);
                        exit(1);

                    }

                    int result = dup2(targetFD, 1);

                    if(result == -1) {
                        perror("dup2");
                        exit(2);

                    }

                    close(targetFD);

                }
            }

            if(execvp(commandLineArguments[0], commandLineArguments)) {
                printf("%s: no such file or directory\n", commandLineArguments[0]);
                fflush(stdout);
                exit(1);
            }


            } else { // pid not equal to 0 or 1 meaning that it is a parent_id
                if(backgroundProcess == 0) { // foreground process
                    child_pid = waitpid(child_pid, &status, 0);

                    if(WIFSIGNALED(status)) { // if it didn't exit normally, it was terminated by a signal
                        printf("terminated by signal %i \n", status);
                        fflush(stdout);

                    }

                } else if(backgroundProcess == 1){ // background process
                    //waitpid(child_pid, &status, WNOHANG);
                    printf("background pid is %i \n", child_pid);
                    fflush(stdout);
                }

            }
        }

        outputFilename = NULL;
        inputFilename = NULL;

        int i = 0;
        for(i = 0; i < arr_index; i++) { // clean up command line arguments for next round of user input
            commandLineArguments[i] = NULL;

        }

        child_pid = waitpid(-1, &status, WNOHANG);

        while(child_pid > 0) {
            if(WIFEXITED(status)) {
                printf("background pid %i is done: terminated by signal %i \n", child_pid, status);
                fflush(stdout);

            } else {
                printf("background pid %i is done: terminated by signal %i \n", status);
                fflush(stdout);
            }

            child_pid = waitpid(-1, &status, WNOHANG);
        }
    }
    return 0;
}
