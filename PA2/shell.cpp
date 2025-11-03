#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <fcntl.h>
#include <cstdlib>
#include <ctime>
#include "Tokenizer.h"
#include "Command.h"
using namespace std;

// color codes for shell prompt
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define WHITE   "\033[1;37m"
#define NC      "\033[0m"

// print timestamped shell prompt
static void printPrompt() {
    time_t now = time(nullptr);
    tm* lt = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%b %d %H:%M:%S", lt);
    cout << BLUE << buf << WHITE << " root:/autograder/source$ " << NC;
    cout.flush();
}

// close all FDs in vector
static void closeAll(vector<int>& fds) {
    for (int fd : fds)
        if (fd != -1) close(fd);
}

// execute pipeline; return exit status of last command
static int runPipeline(const vector<Command*>& commands) {
    int total = (int)commands.size();
    if (total <= 0) return 0;

    vector<int> pipes(2 * max(0, total - 1), -1);
    for (int i = 0; i < total - 1; ++i) {
        if (pipe(&pipes[2 * i]) < 0) { perror("pipe"); return 1; }
    }

    vector<pid_t> child_pids;
    child_pids.reserve(total);

    for (int i = 0; i < total; ++i) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); closeAll(pipes); return 1; }

        if (pid == 0) { // child
            if (i > 0 && dup2(pipes[(i - 1) * 2], STDIN_FILENO) < 0) { perror("dup2 in"); _exit(1); }
            if (i < total - 1 && dup2(pipes[i * 2 + 1], STDOUT_FILENO) < 0) { perror("dup2 out"); _exit(1); }
            closeAll(pipes);

            Command* cmd = commands[i];

            // handle input redirection
            if (cmd->hasInput()) {
                int fd = open(cmd->input_file.c_str(), O_RDONLY);
                if (fd < 0) { perror("open <"); _exit(1); }
                if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2 <"); _exit(1); }
                close(fd);
            }

            // handle output redirection
            if (cmd->hasOutput()) {
                int fd = open(cmd->output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open >"); _exit(1); }
                if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2 >"); _exit(1); }
                close(fd);
            }

            // build argv
            vector<char*> argv;
            argv.reserve(cmd->arguments.size() + 1);
            for (auto& s : cmd->arguments)
                argv.push_back(const_cast<char*>(s.c_str()));
            argv.push_back(nullptr);
            if (!argv[0]) _exit(0);

            execvp(argv[0], argv.data());
            perror("execvp");
            _exit(127);
        }

        // parent
        child_pids.push_back(pid);
        if (i > 0)     { close(pipes[(i - 1) * 2]);   pipes[(i - 1) * 2] = -1; }
        if (i < total - 1) { close(pipes[i * 2 + 1]); pipes[i * 2 + 1] = -1; }
    }

    closeAll(pipes);

    bool background = commands.back()->isBackground();
    int last_code = 0;

    if (!background) {
        for (size_t k = 0; k < child_pids.size(); ++k) {
            int status = 0;
            (void)waitpid(child_pids[k], &status, 0);
            if (k == child_pids.size() - 1) {
                if (WIFEXITED(status)) last_code = WEXITSTATUS(status);
                else last_code = 1;
            }
        }
    } else {
        int status;
        while (waitpid(-1, &status, WNOHANG) > 0) {}
        last_code = 0;
    }

    return last_code;
}

int main() {
    ios::sync_with_stdio(false);

    while (true) {
        printPrompt();
        string line;
        if (!getline(cin, line)) break;
        if (line.empty()) continue;
        if (line == "exit") break;

        bool in_single = false, in_double = false;
        string segment;
        segment.reserve(line.size());

        bool should_run = true;
        int prev_code = 0;

        auto runSegment = [&](const string& seg) {
            int status = 0;
            size_t pos = seg.find_first_not_of(" \t\r\n");
            if (pos == string::npos) return 0;
            if (should_run) {
                if (seg == "exit") return -999;
                Tokenizer tok(seg);
                if (tok.hasError()) status = 1;
                else status = runPipeline(tok.commands);
            }
            return status;
        };

        // parse ; and &&
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '\'' && !in_double) { in_single = !in_single; segment.push_back(c); continue; }
            if (c == '"'  && !in_single) { in_double = !in_double; segment.push_back(c); continue; }

            if (!in_single && !in_double) {
                if (c == ';') {
                    int st = runSegment(segment);
                    if (st == -999) return 0;
                    prev_code = st;
                    should_run = true;
                    segment.clear();
                    continue;
                }
                if (c == '&' && i + 1 < line.size() && line[i + 1] == '&') {
                    int st = runSegment(segment);
                    if (st == -999) return 0;
                    prev_code = st;
                    should_run = (prev_code == 0);
                    segment.clear();
                    ++i;
                    continue;
                }
            }
            segment.push_back(c);
        }

        // run last segment
        if (!segment.empty()) {
            int st = runSegment(segment);
            if (st == -999) return 0;
            prev_code = st;
        }
    }
    return 0;
}
