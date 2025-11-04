/* Nikita Kelwada
    Github : https://github.com/nikita-kelwada/CSCE_313
    CSCE 313 - PA2
*/

#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>
#include <string>

#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

//added for open
#include <fcntl.h>
#include <cstdlib>
#include <ctime>
#include "Command.h"

using namespace std;

//HELPERS 

// close all valid file descriptors
static void closeFDs(vector<int>& fds) {
    for (int fd : fds)
        if (fd != -1) close(fd);
}

// allocate pipes between commands
static bool setupPipes(vector<int>& pipes, int cmdCount) {
    pipes.assign(2 * max(0, cmdCount - 1), -1);
    for (int i = 0; i < cmdCount - 1; ++i) {
        if (pipe(&pipes[2 * i]) < 0) {
            perror("pipe");
            return false;
        }
    }
    return true;
}

// handle input/output redirections (< and >)
static void setupRedirects(Command* cmd) {
    if (cmd->hasInput()) {
        int fd = open(cmd->in_file.c_str(), O_RDONLY);
        if (fd < 0) { perror("open <"); _exit(1); }
        if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2 <"); _exit(1); }
        close(fd);
    }
    if (cmd->hasOutput()) {
        int fd = open(cmd->out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) { perror("open >"); _exit(1); }
        if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2 >"); _exit(1); }
        close(fd);
    }
}

// fork and launch one command from the pipeline
static pid_t spawnChild(Command* cmd, int index, int total, vector<int>& pipes) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }

    if (pid == 0) {
        // link stdin/stdout to pipes as needed
        if (index > 0 && dup2(pipes[(index - 1) * 2], STDIN_FILENO) < 0) { perror("dup2 in"); _exit(1); }
        if (index < total - 1 && dup2(pipes[index * 2 + 1], STDOUT_FILENO) < 0) { perror("dup2 out"); _exit(1); }

        closeFDs(pipes);
        setupRedirects(cmd);

        // prepare args for execvp
        vector<char*> argv;
        for (auto& s : cmd->args) argv.push_back(const_cast<char*>(s.c_str()));
        argv.push_back(nullptr);
        if (!argv[0]) _exit(0);

        // execvp only returns if it fails
        execvp(argv[0], argv.data());
        perror("execvp");

        // 127 is the conventional "command not found" exit code in UNIX
        //self note : or would replacing w/ #define be better, or not make a diff
        _exit(127);
    }

    return pid;
}

// wait for all processes in a pipeline or background job
static int waitForChildren(vector<pid_t>& pids, bool background) {
    int status = 0, last = 0;

    if (!background) {
        for (size_t i = 0; i < pids.size(); ++i) {
            waitpid(pids[i], &status, 0);
            if (i == pids.size() - 1)
                last = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
    } else {
        // background jobs are not waited for normally
        // clears any finished children to avoid zombies
        while (waitpid(-1, &status, WNOHANG) > 0);
        last = 0;
    }

    return last;
}

//HELPERS END

// run a full pipeline of commands and return the last command’s exit code
static int execute(const vector<Command*>& cmds) {
    int n = (int)cmds.size();
    if (n <= 0) return 0;

    vector<int> pipes;
    if (!setupPipes(pipes, n)) return 1;

    vector<pid_t> pids;
    pids.reserve(n);

    for (int i = 0; i < n; ++i) {
        pid_t pid = spawnChild(cmds[i], i, n, pipes);
        if (pid < 0) { closeFDs(pipes); return 1; }
        pids.push_back(pid);

        // close unused pipe ends early to free file descriptors
        if (i > 0) close(pipes[(i - 1) * 2]);
        if (i < n - 1) close(pipes[i * 2 + 1]);
    }

    closeFDs(pipes);
    bool background = cmds.back()->isBackground();
    return waitForChildren(pids, background);
}

// print timestamped shell prompt
static void printPrompt() {
    time_t now = time(nullptr);
    tm* lt = localtime(&now);
    char tb[32];
    strftime(tb, sizeof(tb), "%b %d %H:%M:%S", lt);
    cout << tb << " root:/autograder/source$ ";
    cout.flush();
}

int main() {
    ios::sync_with_stdio(false);

    while (true) {
        printPrompt();

        string line;
        if (!getline(cin, line)) break;
        if (line.empty()) continue;

        bool inSingle = false, inDouble = false;
        bool runNext = true;
        string segment;

        // -999 is just a sentinel code meaning "user typed exit"
        auto run = [&](const string& cmd) -> int {
            if (!runNext) return 0;
            if (cmd == "exit") return -999;
            Tokenizer t(cmd);
            if (t.hasError()) return 1;
            return execute(t.commands);
        };

        // parse line manually so we can handle quotes and short-circuit logic
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];

            if (c == '\'' && !inDouble) inSingle = !inSingle;
            else if (c == '"' && !inSingle) inDouble = !inDouble;
            else if (!inSingle && !inDouble) {
                // handle ';' or '&&' operators
                if (c == ';' || (c == '&' && i + 1 < line.size() && line[i + 1] == '&')) {
                    int st = run(segment);
                    if (st == -999) return 0;

                    // only run next command if appropriate
                    runNext = (c == ';') || (st == 0);

                    segment.clear();
                    if (c == '&') ++i; // skip second &
                    continue;
                }
            }

            segment += c;
        }

        // execute final command (if any)
        if (!segment.empty()) {
            int st = run(segment);
            if (st == -999) return 0;
        }
    }

    return 0;
}
