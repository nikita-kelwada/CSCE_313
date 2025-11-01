#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>

class Command {
public:
    // Raw command string for this pipeline segment (from Tokenizer)
    std::string cmd;

    // Final argv after parsing (argv[0] is the program)
    std::vector<std::string> args;

    // Quoted-substring store from Tokenizer
    std::vector<std::string> inner_strings;

    // Flags and files for I/O/background
    bool bg = false;
    std::string in_file;
    std::string out_file;
    bool append_out = false;   // NEW: support for >>

    // Construct from a raw pipeline-segment string and the shared inner_strings
    Command(const std::string _cmd, std::vector<std::string> _inner_strings);

    // Accessors used by the shell
    bool hasInput()     const { return !in_file.empty(); }
    bool hasOutput()    const { return !out_file.empty(); }
    bool isBackground() const { return bg; }

private:
    // Helpers
    static std::string trim(const std::string& s);
    void parse();                // tokenizes and fills args/in/out/bg
    std::string restorePlaceholders(const std::string& tok) const;
};

#endif
