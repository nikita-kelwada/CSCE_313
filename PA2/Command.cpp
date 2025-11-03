#include "Command.h"
#include <sstream>
#include <algorithm>
#include <cctype>
using namespace std;

Command::Command(const string cmd_str, vector<string> inner_strs) {
    // trim whitespace
    raw_cmd = trim(cmd_str);

    // detect background
    run_background = (raw_cmd.size() > 0 && raw_cmd.substr(raw_cmd.size() - 1) == "&");

    input_file = "";
    output_file = "";
    inner_segments = inner_strs;

    // handle I/O redirection
    locateRedirection();

    // parse arguments
    tokenizeArguments();
}

bool Command::hasInput() {
    return input_file != "";
}

bool Command::hasOutput() {
    return output_file != "";
}

bool Command::isBackground() {
    return run_background;
}

// remove leading/trailing spaces
string Command::trim(const string str) {
    int i = str.find_first_not_of(" \n\r\t");
    int j = str.find_last_not_of(" \n\r\t");
    if (i >= 0 && j >= i) return str.substr(i, j - i + 1);
    return str;
}

// find < or > and extract filenames
void Command::locateRedirection() {
    // input
    if (raw_cmd.find("<") != string::npos) {
        int start = raw_cmd.find("<");
        int end = raw_cmd.find_first_of(" \n\r\t>", raw_cmd.find_first_not_of(" \n\r\t", start + 1));
        if ((size_t)end == string::npos) end = raw_cmd.size();

        input_file = trim(raw_cmd.substr(start + 1, end - start - 1));
        raw_cmd = trim(raw_cmd.substr(0, start) + raw_cmd.substr(end));
    }

    // output
    if (raw_cmd.find(">") != string::npos) {
        int start = raw_cmd.find(">");
        int end = raw_cmd.find_first_of(" \n\r\t<", raw_cmd.find_first_not_of(" \n\r\t", start + 1));
        if ((size_t)end == string::npos) end = raw_cmd.size();

        output_file = trim(raw_cmd.substr(start + 1, end - start - 1));
        raw_cmd = trim(raw_cmd.substr(0, start) + raw_cmd.substr(end));
    }
}

// tokenize command and handle --strN replacements
void Command::tokenizeArguments() {
    string temp = raw_cmd;
    string delim = " ";
    size_t idx = 0;

    // split by spaces
    while ((idx = temp.find(delim)) != string::npos) {
        arguments.push_back(trim(temp.substr(0, idx)));
        temp = trim(temp.substr(idx + 1));
    }
    arguments.push_back(trim(temp));

    // remove '&' if background
    if (run_background && !arguments.empty() && arguments.back() == "&") arguments.pop_back();

    // add --color=auto for ls/grep
    int insert_pos = 1;
    if (!arguments.empty() && (arguments.at(0) == "ls" || arguments.at(0) == "grep")) insert_pos = 2;

    idx = 0;
    while (idx < arguments.size()) {
        if (arguments.at(idx) == "--str") {
            if (idx + 1 < arguments.size()) {
                int ref = -1;
                try { ref = stoi(arguments.at(idx + 1)); } catch (...) { ref = -1; }

                if (ref >= 0 && (size_t)ref < inner_segments.size())
                    arguments.at(idx) = inner_segments.at(ref);
                else
                    arguments.at(idx) = "";
                arguments.erase(arguments.begin() + idx + 1);
            } else {
                arguments.erase(arguments.begin() + idx);
                continue;
            }
        } else {
            // embedded --strN
            size_t pos = arguments.at(idx).find("--str");
            if (pos != string::npos) {
                size_t end = pos + 5;
                while (end < arguments.at(idx).size() && isdigit((unsigned char)arguments.at(idx)[end])) end++;
                if (end > pos + 5) {
                    int ref = -1;
                    try { ref = stoi(arguments.at(idx).substr(pos + 5, end - (pos + 5))); } catch (...) { ref = -1; }

                    string replacement = "";
                    if (ref >= 0 && (size_t)ref < inner_segments.size())
                        replacement = inner_segments.at(ref);

                    arguments.at(idx).replace(pos, end - pos, replacement);
                }
            }
        }
        idx++;
    }

    // insert color flag if needed
    if (insert_pos > 1 && arguments.size() >= 1)
        arguments.insert(arguments.begin() + 1, "--color=auto");
}
