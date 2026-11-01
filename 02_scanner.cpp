/*
 * QUESTION:
 * 2) Write a Lexical Analyzer (Scanner) in C, C++, or Java that reads a
 *    source code file of a subset language and breaks it down into a stream
 *    of lexemes and valid tokens. Show the output in tabular form.
 *    Your program must discard whitespace and comments while tracking line
 *    numbers for error reporting.
 *
 * Subset language : mini-C (keywords int float char void if else while for
 *   return print; identifiers; int/float constants; string literals;
 *   arithmetic / relational / logical operators; delimiters; // and block
 *   comments)
 *
 * Build : g++ -std=c++17 -Wall -o scanner 02_scanner.cpp
 * Run   : ./scanner sample_input.src      (or sample_errors.src)
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>
#include <cctype>
using namespace std;

struct Token { int line; string type, lexeme; };

class Scanner {
    string src;
    size_t pos = 0;
    int line = 1;
    vector<Token> tokens;
    vector<string> errors;

    static const set<string> keywords;
    static const vector<string> twoCharOps;

    char peek(size_t k = 0) const { return pos + k < src.size() ? src[pos + k] : '\0'; }
    void add(const string &type, const string &lex) { tokens.push_back({line, type, lex}); }
    void error(int ln, const string &msg, const string &text) {
        errors.push_back("Line " + to_string(ln) + ": lexical error - " + msg + " '" + text + "'");
    }

public:
    explicit Scanner(string text) : src(move(text)) {}
    const vector<Token> &getTokens() const { return tokens; }
    const vector<string> &getErrors() const { return errors; }
    int lines() const { return line; }

    void run() {
        while (peek()) {
            char c = peek();
            if (c == '\n') { line++; pos++; continue; }                 // whitespace
            if (isspace((unsigned char)c)) { pos++; continue; }

            if (c == '/' && peek(1) == '/') {                           // // comment
                while (peek() && peek() != '\n') pos++;
                continue;
            }
            if (c == '/' && peek(1) == '*') {                           // block comment
                int startLine = line;
                pos += 2;
                while (peek() && !(peek() == '*' && peek(1) == '/')) {
                    if (peek() == '\n') line++;
                    pos++;
                }
                if (!peek()) error(startLine, "unterminated comment starting at", "/*");
                else pos += 2;
                continue;
            }

            size_t start = pos;
            if (isalpha((unsigned char)c) || c == '_') {                // id / keyword
                while (isalnum((unsigned char)peek()) || peek() == '_') pos++;
                string lex = src.substr(start, pos - start);
                add(keywords.count(lex) ? "KEYWORD" : "IDENTIFIER", lex);
            } else if (isdigit((unsigned char)c)) {                     // numbers
                bool isFloat = false;
                while (isdigit((unsigned char)peek())) pos++;
                if (peek() == '.' && isdigit((unsigned char)peek(1))) {
                    isFloat = true; pos++;
                    while (isdigit((unsigned char)peek())) pos++;
                }
                if (isalpha((unsigned char)peek()) || peek() == '_') {
                    while (isalnum((unsigned char)peek()) || peek() == '_') pos++;
                    error(line, "malformed number / identifier starts with digit",
                          src.substr(start, pos - start));
                } else {
                    add(isFloat ? "FLOAT_CONST" : "INT_CONST", src.substr(start, pos - start));
                }
            } else if (c == '"') {                                      // string
                pos++;
                while (peek() && peek() != '"' && peek() != '\n') pos++;
                if (peek() == '"') { pos++; add("STRING", src.substr(start, pos - start)); }
                else error(line, "unterminated string literal", "\"");
            } else {                                                    // operators etc.
                bool matched = false;
                for (const string &op : twoCharOps)
                    if (src.compare(pos, 2, op) == 0) {
                        add("OPERATOR", op); pos += 2; matched = true; break;
                    }
                if (!matched) {
                    string one(1, c);
                    if (string("+-*/%=<>!&|").find(c) != string::npos) add("OPERATOR", one);
                    else if (string("(){}[];,").find(c) != string::npos) add("DELIMITER", one);
                    else error(line, "illegal character", one);
                    pos++;
                }
            }
        }
    }
};
const set<string> Scanner::keywords = {"int", "float", "char", "void", "if", "else",
                                       "while", "for", "return", "print"};
const vector<string> Scanner::twoCharOps = {"==", "!=", "<=", ">=", "&&", "||", "++",
                                            "--", "+=", "-=", "*=", "/="};

int main(int argc, char *argv[]) {
    if (argc < 2) { cerr << "Usage: " << argv[0] << " <source-file>\n"; return 1; }
    ifstream f(argv[1]);
    if (!f) { cerr << "cannot open " << argv[1] << "\n"; return 1; }
    stringstream ss; ss << f.rdbuf();

    Scanner sc(ss.str());
    sc.run();

    const string border = "+------+-----+--------------+----------------------+\n";
    cout << border << "| LINE | NO. | TOKEN        | LEXEME               |\n" << border;
    int n = 0;
    for (const Token &t : sc.getTokens())
        cout << "| " << left << setw(4) << t.line << " | " << setw(3) << ++n << " | "
             << setw(12) << t.type << " | " << setw(20) << t.lexeme << " |\n";
    cout << border << "Total tokens: " << n << ", lexical errors: " << sc.getErrors().size()
         << ", lines: " << sc.lines() << "\n";
    for (const string &e : sc.getErrors()) cout << e << "\n";
    return sc.getErrors().empty() ? 0 : 2;
}
