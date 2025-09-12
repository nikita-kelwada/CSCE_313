/****************
LE2: Introduction to Unnamed Pipes
Nikita Kelwada
CSCE 313 - Section 512
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
#include <sys/wait.h> // MY NOTE ; added for waitpid
#include <iostream> //MY NOTE : added for cout
using namespace std;

int main () {
    // lists all the files in the root directory in the long format
    char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};
    // translates all input from lowercase to uppercase
    char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};

    // TODO: add functionality
    // Create pipe
    int pipeEnds[2]; // MY NOTE : Pipes are used for inter-process communication
    if (pipe(pipeEnds) == -1) {
        perror("pipe");
        return 1;
    }

    // Create child to run first command
    pid_t listChild = fork();
    if (listChild == -1) {
        perror("fork");
        return 1;
    }

    if (listChild == 0) {
        // Inside the child, redirect output to write end of pipe
        // close unused read end
        close(pipeEnds[0]);  // MY NOTE : This will close the read end of the pipe in the child process
        // stdout -> write end
        dup2(pipeEnds[1], STDOUT_FILENO);
        // close after dup 
        close(pipeEnds[1]); 
    
        // In child, execute the command
        execvp(cmd1[0], cmd1);
        perror("execvp ls");
        _exit(1);
    }

    // Create another child to run second command
    // MY NOTE : Fork a second child process to execute 'tr a-z A-Z'.
    pid_t childTranslate = fork();
    if (childTranslate == -1) { 
        perror("fork");
        return 1;
    }

    if (childTranslate == 0) {
        // In child, redirect input to the read end of the pipe
        // close unused write end of the pipe on the child side
        close(pipeEnds[1]);                   
        dup2(pipeEnds[0], STDIN_FILENO);         
        close(pipeEnds[0]);                       

        // Execute the second command.
        execvp(cmd2[0], cmd2);
        perror("execvp tr");
        _exit(1);
    }

    // Reset the input and output file descriptors of the parent.
    // MY NOTE : This will close unused pipe ends in the parent process
    close(pipeEnds[0]);
    close(pipeEnds[1]);

    // MY NOTE : This will make the process wait for boththe children to finish
    waitpid(listChild, nullptr, 0);
    waitpid(childTranslate, nullptr, 0);

    return 0;
}
