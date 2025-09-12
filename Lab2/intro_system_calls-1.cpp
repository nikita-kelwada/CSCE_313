/*
    ============================================================
    Introduction to fork(), exec(), pipe(), and wait()
    ============================================================
    This program is a teaching example that demonstrates how 
    basic UNIX system calls work. It is NOT meant to solve 
    a particular problem — instead, it shows three small demos:

      1. fork()  → how a parent and child process are created.
      2. exec()  → how a process can replace itself with another 
                   program (for example, running "ls").
      3. pipe()  → how two processes can communicate by passing 
                   data through a pipe (child writes, parent reads).

    Each section includes clear printouts so you can see the 
    difference between parent and child processes, and how data 
    moves between them. Use this code as a learning reference 
    to understand system calls, not as a final solution to a lab.
*/

#include <unistd.h>   // fork, execvp, pipe, read, write, close
#include <sys/wait.h> // wait, waitpid
#include <iostream>   // std::cout, std::cerr

using namespace std;

int main() {
    // ============================================================
    // DEMO 1: fork()
    // ============================================================
    cout << "=== DEMO: fork() ===" << endl;

    // fork() creates a new process by duplicating the current one
    // - Parent gets the child’s PID (a positive number)
    // - Child gets 0
    pid_t pid = fork();

    if (pid == -1) {
        cerr << "fork failed\n";
        return 1;
    }

    if (pid == 0) {
        // This branch runs in the CHILD process
        cout << "Hello from the CHILD process! (pid=" << getpid() << ")\n";
        return 0; // child exits here
    } else {
        // This branch runs in the PARENT process
        cout << "Hello from the PARENT process! (pid=" << getpid()
             << "), child=" << pid << "\n";

        // wait() makes the parent pause until the child finishes
        wait(nullptr);
    }

    // ============================================================
    // DEMO 2: exec()
    // ============================================================
    cout << "\n=== DEMO: exec() ===" << endl;
    cout << "Now the child will be REPLACED by another program (\"ls\")\n";

    pid = fork();
    if (pid == -1) {
        cerr << "fork failed\n";
        return 1;
    }

    if (pid == 0) {
        // In the CHILD process
        // Prepare arguments for "ls -1"
        // Note: argv must end with nullptr
        char* args[] = {(char*)"ls", (char*)"-1", nullptr};

        // execvp() REPLACES this process with "ls"
        execvp(args[0], args);

        // If execvp returns, something went wrong
        cerr << "exec failed\n";
        return 1;
    } else {
        // PARENT waits again for the child to finish
        wait(nullptr);
    }

    // ============================================================
    // DEMO 3: pipe()
    // ============================================================
    cout << "\n=== DEMO: pipe() ===" << endl;

    // A pipe is a unidirectional data channel
    // fds[0] = read end, fds[1] = write end
    int fds[2];
    if (pipe(fds) == -1) {
        cerr << "pipe failed\n";
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        cerr << "fork failed\n";
        return 1;
    }

    if (pid == 0) {
        // CHILD process writes into the pipe

        close(fds[0]); // close unused read end

        const char msg[] = "Message from CHILD process\n";
        // write data into the pipe
        write(fds[1], msg, sizeof(msg));

        close(fds[1]); // close write end when done
        return 0;
    } else {
        // PARENT process reads from the pipe

        close(fds[1]); // close unused write end

        char buffer[100];              // space to store message
        int n = read(fds[0], buffer, sizeof(buffer));
        buffer[n] = '\0';              // null-terminate string

        cout << "Parent received: " << buffer;

        close(fds[0]); // close read end
        wait(nullptr); // wait for child
    }

    // ============================================================
    // End of demo
    // ============================================================
    cout << "\n=== END OF DEMO ===" << endl;
    return 0;
}
