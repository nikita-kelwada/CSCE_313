#include "Command.h"
#include <sstream>
#include <cctype>
#include <algorithm>

using namespace std;

static bool is_digits(const std::string& s) { //helper to check if string contains only digits
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

Command::Command(const std::string _cmd, std::vector<std::string> _inner_strings)
    : cmd(_cmd), inner_strings(std::move(_inner_strings)) {
    parse();
}

std::string Command::trim(const std::string& s) { // helper to trim whitespace from both ends of a string
    size_t i = s.find_first_not_of(" \t\r\n");
    if (i == string::npos) return "";
    size_t j = s.find_last_not_of(" \t\r\n");
    return s.substr(i, j - i + 1);
}

// Replace any appearance of --str <N> or --str<N> (possibly embedded in a token)
// with the corresponding inner_strings[N]. Works even if multiple occur in one token.
std::string Command::restorePlaceholders(const std::string& tok) const {
    std::string out;
    out.reserve(tok.size());

    for (size_t i = 0; i < tok.size();) {
        // Look for "--str"
        if (i + 5 <= tok.size() && tok.compare(i, 5, "--str") == 0) {
            size_t j = i + 5;

            // Case A: "--str" immediately followed by digits (e.g., --str0)
            if (j < tok.size() && isdigit(static_cast<unsigned char>(tok[j]))) {
                size_t k = j;
                while (k < tok.size() && isdigit(static_cast<unsigned char>(tok[k]))) k++;
                int idx = stoi(tok.substr(j, k - j));
                out += (0 <= idx && (size_t)idx < inner_strings.size()) ? inner_strings[idx] : "";
                i = k;
                continue;
            }
            // Case B: token equals "--str" and digits come as separate token.
            // (Handled in parse() when looking at neighbors.)
        }

        out.push_back(tok[i]);
        i++;
    }

    return out;
}

void Command::parse() {
    // First pass: whitespace tokenization
    vector<string> raw;
    {
        istringstream ss(cmd);
        string t;
        while (ss >> t) raw.push_back(t);
    }

    vector<string> tmp_args; // temporary arguments vector
    for (size_t i = 0; i < raw.size(); ++i) { // iterate over each raw token
        string t = raw[i];

        // Handle the Tokenizer pattern from -d' ' → "-d--str 0"
        // Split into "-d" and "--str..."
        if (t.rfind("-d--str", 0) == 0) {
            tmp_args.push_back("-d");
            // remainder becomes a synthetic token to process (may be "--str" or "--str<digits>")
            t = string("--str") + t.substr(7); // 7 == len("-d--str")
            // continue processing 't' below
        }

        // Background
        if (t == "&") { bg = true; continue; }

        //will handle output redirection (append)
        if (t.size() >= 3 && t.rfind(">>", 0) == 0) {
            out_file   = restorePlaceholders(t.substr(2));
            append_out = true;
            continue;
        }
        // Handle spaced form: ">> file"
        if (t == ">>") {
            if (i + 1 < raw.size()) {
                out_file   = restorePlaceholders(raw[++i]);
                append_out = true;
            }
            continue;
        }

        // will handle output redirection (truncate)
        if (t.size() >= 2 && t[0] == '>' && t[1] != '>') {
            out_file   = restorePlaceholders(t.substr(1));
            append_out = false;
            continue;
        }
        // Handle spaced form: "> file"
        if (t == ">") {
            if (i + 1 < raw.size()) {
                out_file   = restorePlaceholders(raw[++i]);
                append_out = false;
            }
            continue;
        }

        //will handle input redirection
        if (t.size() >= 2 && t[0] == '<') {
            in_file = restorePlaceholders(t.substr(1));
            continue;
        }
        // Handle spaced form: "< file"
        if (t == "<") {
            if (i + 1 < raw.size()) in_file = restorePlaceholders(raw[++i]);
            continue;
        }

        // Pair form: "--str" <digits>
        if (t == "--str" && i + 1 < raw.size() && is_digits(raw[i + 1])) {
            int idx = stoi(raw[i + 1]);
            tmp_args.push_back((0 <= idx && (size_t)idx < inner_strings.size())
                                ? inner_strings[idx] : "");
            ++i; // consume the digits token
            continue;
        }

        // Embedded/suffixed form in this token (e.g., abc--str3xyz or --str7)
        tmp_args.push_back(restorePlaceholders(t));
    }

    // Drop accidental ";" if present mid-statement; keep only argv for exec
    vector<string> keep;
    keep.reserve(tmp_args.size());
    for (const auto& s : tmp_args) {
        if (s == ";") break;
        if (!s.empty()) keep.push_back(s);
    }

    args.swap(keep);
}
