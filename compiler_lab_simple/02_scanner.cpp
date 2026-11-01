/*
 * QUESTION:
 * 2) Write a Lexical Analyzer (Scanner) in C, C++, or Java that reads a
 *    source code file of a subset language and breaks it down into a stream
 *    of lexemes and valid tokens. Show the output in tabular form.
 *    Your program must discard whitespace and comments while tracking line
 *    numbers for error reporting.
 *
 * Subset language: keywords (int float char void if else while for return
 * print), identifiers, int/float constants, strings, operators, delimiters,
 * // and block comments.
 *
 * Build : g++ -o scanner 02_scanner.cpp
 * Run   : ./scanner sample_input.src        (or sample_errors.src)
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cctype>
using namespace std;

bool isKeyword(string w) {
    string kw[] = {"int", "float", "char", "void", "if", "else", "while", "for", "return", "print"};
    for (int i = 0; i < 10; i++) if (w == kw[i]) return true;
    return false;
}

int tokenCount = 0;
void show(int line, string type, string lexeme) {
    cout << "| " << left << setw(4) << line << " | " << setw(3) << ++tokenCount << " | "
         << setw(12) << type << " | " << setw(20) << lexeme << " |\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) { cout << "Usage: ./scanner <file>\n"; return 1; }
    ifstream f(argv[1]);
    if (!f) { cout << "cannot open file\n"; return 1; }
    stringstream ss;
    ss << f.rdbuf();
    string s = ss.str();

    vector<string> errors;
    int line = 1;
    size_t i = 0;
    string border = "+------+-----+--------------+----------------------+\n";
    cout << border << "| LINE | NO. | TOKEN        | LEXEME               |\n" << border;

    while (i < s.size()) {
        char c = s[i];
        char next = (i + 1 < s.size()) ? s[i + 1] : '\0';

        if (c == '\n') { line++; i++; }                           // whitespace
        else if (isspace(c)) i++;
        else if (c == '/' && next == '/') {                       // // comment
            while (i < s.size() && s[i] != '\n') i++;
        }
        else if (c == '/' && next == '*') {                       // block comment
            int start = line;
            i += 2;
            while (i + 1 < s.size() && !(s[i] == '*' && s[i + 1] == '/')) {
                if (s[i] == '\n') line++;
                i++;
            }
            if (i + 1 >= s.size()) { errors.push_back("Line " + to_string(start) + ": unterminated comment"); i = s.size(); }
            else i += 2;
        }
        else if (isalpha(c) || c == '_') {                        // identifier / keyword
            size_t st = i;
            while (i < s.size() && (isalnum(s[i]) || s[i] == '_')) i++;
            string w = s.substr(st, i - st);
            show(line, isKeyword(w) ? "KEYWORD" : "IDENTIFIER", w);
        }
        else if (isdigit(c)) {                                    // number
            size_t st = i;
            bool isFloat = false;
            while (i < s.size() && isdigit(s[i])) i++;
            if (i + 1 < s.size() && s[i] == '.' && isdigit(s[i + 1])) {
                isFloat = true;
                i++;
                while (i < s.size() && isdigit(s[i])) i++;
            }
            if (i < s.size() && (isalpha(s[i]) || s[i] == '_')) {  // like 2abc
                while (i < s.size() && (isalnum(s[i]) || s[i] == '_')) i++;
                errors.push_back("Line " + to_string(line) + ": malformed token '" + s.substr(st, i - st) + "'");
            } else show(line, isFloat ? "FLOAT_CONST" : "INT_CONST", s.substr(st, i - st));
        }
        else if (c == '"') {                                      // string
            size_t st = i++;
            while (i < s.size() && s[i] != '"' && s[i] != '\n') i++;
            if (i < s.size() && s[i] == '"') { i++; show(line, "STRING", s.substr(st, i - st)); }
            else errors.push_back("Line " + to_string(line) + ": unterminated string");
        }
        else {                                                    // operators, delimiters
            string two = s.substr(i, 2);
            if (two == "==" || two == "!=" || two == "<=" || two == ">=" || two == "&&" || two == "||") {
                show(line, "OPERATOR", two);
                i += 2;
            } else {
                string one(1, c);
                if (string("+-*/%=<>!").find(c) != string::npos) show(line, "OPERATOR", one);
                else if (string("(){}[];,").find(c) != string::npos) show(line, "DELIMITER", one);
                else errors.push_back("Line " + to_string(line) + ": illegal character '" + one + "'");
                i++;
            }
        }
    }
    cout << border << "Total tokens: " << tokenCount << ", errors: " << errors.size() << "\n";
    for (size_t k = 0; k < errors.size(); k++) cout << "Error - " << errors[k] << "\n";
}
