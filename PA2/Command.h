#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>

class Command {
public:
    // raw command string (one pipeline segment)
    std::string raw_cmd;

    // parsed argument list
    std::vector<std::string> arguments;

    // inner quoted strings from tokenizer
    std::vector<std::string> inner_segments;

    // I/O and background flags
    bool run_background = false;  // true if '&' present
    std::string input_file;       // input redirection
    std::string output_file;      // output redirection
    bool append_output = false;   // true if ">>" used

    // constructor
    Command(const std::string cmd_str, std::vector<std::string> inner_strs);

    // accessors
    bool hasInput();
    bool hasOutput();
    bool isBackground();

private:
    // helpers for trimming and parsing
    std::string trim(const std::string str);
    void locateRedirection();
    void tokenizeArguments();
};

#endif
