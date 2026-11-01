/*
 * QUESTION:
 * 1) Write a program that implements a two-buffer input system to read an
 *    input source file. Your program must efficiently manage buffer pairs,
 *    handle lookahead pointers, detect lexemes (like keywords and
 *    identifiers), and handle end-of-buffer conditions using sentinels.
 *
 * IDEA (N = size of one half)
 *   index: 0 .. N-1 | N | N+1 .. 2N | 2N+1
 *          half 1   | S | half 2    | S        S = sentinel '\0'
 *   lexemeBegin = start of the current lexeme, fwd = lookahead pointer.
 *   When fwd reaches a sentinel at the end of a half, the other half is
 *   reloaded from the file. A sentinel inside a half means end of file.
 *   (A lexeme must be shorter than N characters.)
 *
 * Build : g++ -o two_buffer 01_two_buffer_input.cpp
 * Run   : ./two_buffer sample_input.src
 */
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cctype>
#include <cstring>
using namespace std;

#define N 16                        // size of each half (small, to see reloads)
char buf[2 * N + 2];
char *lexemeBegin, *fwd;            // fwd = forward (lookahead) pointer
ifstream in;
int reloads = 0, line = 1;

void fillHalf(char *start) {        // read N bytes, then put the sentinel
    in.read(start, N);
    int n = in.gcount();
    start[n] = '\0';
    reloads++;
    cout << "   [buffer reload] " << n << " bytes read\n";
}

int nextChar() {                    // returns next char, or -1 at end of file
    char c = *fwd++;
    if (c != '\0') return (unsigned char)c;
    char *s = fwd - 1;              // sentinel position
    if (s == buf + N)         { fillHalf(buf + N + 1); fwd = buf + N + 1; return nextChar(); }
    if (s == buf + 2 * N + 1) { fillHalf(buf);         fwd = buf;         return nextChar(); }
    fwd = s;                        // real end of file
    return -1;
}

void retract() { fwd--; }           // go back one character

string getLexeme() {                // copy lexemeBegin..fwd, skipping sentinels
    string s;
    char *p = lexemeBegin;
    while (p != fwd) {
        if (p == buf + N) p = buf + N + 1;
        else if (p == buf + 2 * N + 1) p = buf;
        else s += *p++;
    }
    return s;
}

bool isKeyword(string w) {
    string kw[] = {"int", "float", "char", "void", "if", "else", "while", "for", "return", "print"};
    for (int i = 0; i < 10; i++) if (w == kw[i]) return true;
    return false;
}

void show(string type, string lexeme) {
    cout << left << setw(6) << line << setw(13) << type << lexeme << "\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) { cout << "Usage: ./two_buffer <file>\n"; return 1; }
    in.open(argv[1], ios::binary);
    if (!in) { cout << "cannot open file\n"; return 1; }

    buf[N] = buf[2 * N + 1] = '\0';
    fillHalf(buf);
    lexemeBegin = fwd = buf;

    cout << "\nLINE  TOKEN        LEXEME\n----------------------------------\n";
    int c = nextChar();
    while (c != -1) {
        if (c == '\n') { line++; c = nextChar(); continue; }
        if (isspace(c)) { c = nextChar(); continue; }
        lexemeBegin = fwd - 1;                       // first char of the lexeme

        if (isalpha(c) || c == '_') {                // identifier or keyword
            while (c != -1 && (isalnum(c) || c == '_')) c = nextChar();
            if (c != -1) retract();
            string w = getLexeme();
            show(isKeyword(w) ? "KEYWORD" : "IDENTIFIER", w);
        } else if (isdigit(c)) {                     // number
            while (c != -1 && (isdigit(c) || c == '.')) c = nextChar();
            if (c != -1) retract();
            show("NUMBER", getLexeme());
        } else {                                     // operator / delimiter / comment
            int first = c;
            c = nextChar();
            if (first == '/' && c == '/') {          // line comment
                while (c != -1 && c != '\n') c = nextChar();
                continue;
            }
            if (first == '/' && c == '*') {          // block comment
                int prev = 0;
                c = nextChar();
                while (c != -1 && !(prev == '*' && c == '/')) {
                    if (c == '\n') line++;
                    prev = c;
                    c = nextChar();
                }
                c = nextChar();
                continue;
            }
            bool twoChars = (c == '=' && strchr("=!<>", first));
            if (!twoChars && c != -1) retract();
            show(strchr("(){}[];,", first) ? "DELIMITER" : "OPERATOR", getLexeme());
        }
        c = nextChar();
    }
    cout << "----------------------------------\nBuffer reloads: " << reloads << "\n";
}
