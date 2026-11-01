/*
 * QUESTION:
 * 1) Write a program that implements a two-buffer input system to read an
 *    input source file. Your program must efficiently manage buffer pairs,
 *    handle lookahead pointers, detect lexemes (like keywords and
 *    identifiers), and handle end-of-buffer conditions using sentinels.
 *
 * DESIGN (N = BUF_SIZE)
 *   index: 0 .. N-1 | N | N+1 .. 2N | 2N+1
 *          half 1   | S | half 2    | S        S = sentinel '\0'
 *   - lexemeBegin : start of the lexeme being recognised
 *   - forward     : lookahead pointer
 *   - When forward reaches a boundary sentinel the OTHER half is reloaded.
 *   - A sentinel that is not at a boundary marks the real end of file.
 *   Only one comparison (is it the sentinel?) is needed per character.
 *   Limitation of the scheme: a lexeme must be shorter than one half.
 *
 * Build : g++ -std=c++17 -Wall -o two_buffer 01_two_buffer_input.cpp
 * Run   : ./two_buffer sample_input.src
 * (-DBUF_SIZE=64 changes the half size; default 16 so reloads are visible.)
 */
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <set>
#include <cctype>
#include <cstring>
using namespace std;

#ifndef BUF_SIZE
#define BUF_SIZE 16
#endif

class TwoBufferInput {
    static const int N = BUF_SIZE;
    static const char SENTINEL = '\0';
    char buf[2 * N + 2];
    ifstream in;
    char *half1() { return buf; }
    char *half2() { return buf + N + 1; }
    char *end1()  { return buf + N; }          // sentinel after half 1
    char *end2()  { return buf + 2 * N + 1; }  // sentinel after half 2

    void fillHalf(char *start) {
        in.read(start, N);
        streamsize n = in.gcount();
        start[n] = SENTINEL;       // boundary sentinel, or EOF mark if n < N
        reloads++;
        cout << "   [buffer] reloaded " << (start == half1() ? "first" : "second")
             << " half (" << n << " bytes read)\n";
    }

public:
    char *lexemeBegin = nullptr, *forward = nullptr;
    int reloads = 0;
    static const int END_OF_INPUT = -1;

    bool open(const string &name) {
        in.open(name, ios::binary);
        if (!in) return false;
        buf[N] = buf[2 * N + 1] = SENTINEL;
        fillHalf(half1());
        lexemeBegin = forward = half1();
        return true;
    }

    int nextChar() {
        char c = *forward++;
        if (c != SENTINEL) return (unsigned char)c;
        char *s = forward - 1;
        if (s == end1()) { fillHalf(half2()); forward = half2(); return nextChar(); }
        if (s == end2()) { fillHalf(half1()); forward = half1(); return nextChar(); }
        forward = s;                       // real EOF: stay on the sentinel
        return END_OF_INPUT;
    }

    void retract() {
        forward--;
        if (forward == end1()) forward = end1() - 1;
        else if (forward == buf - 1) forward = end2() - 1;
    }

    string getLexeme() {                   // skips sentinels between halves
        string s;
        char *p = lexemeBegin;
        while (p != forward) {
            if (p == end1()) { p = half2(); continue; }
            if (p == end2()) { p = half1(); continue; }
            s += *p++;
        }
        return s;
    }
};

static const set<string> KEYWORDS = {"int", "float", "char", "void", "if", "else",
                                     "while", "for", "return", "print"};
static int lineNo = 1;

static void emit(const string &type, const string &lexeme) {
    cout << left << setw(6) << lineNo << setw(13) << type << lexeme << "\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) { cerr << "Usage: " << argv[0] << " <source-file>\n"; return 1; }
    TwoBufferInput tb;
    if (!tb.open(argv[1])) { cerr << "cannot open " << argv[1] << "\n"; return 1; }
    const int EOI = TwoBufferInput::END_OF_INPUT;

    cout << "\n" << left << setw(6) << "LINE" << setw(13) << "TOKEN" << "LEXEME\n"
         << string(34, '-') << "\n";

    int c = tb.nextChar();
    while (c != EOI) {
        if (c == '\n') { lineNo++; c = tb.nextChar(); continue; }
        if (isspace(c)) { c = tb.nextChar(); continue; }

        tb.lexemeBegin = tb.forward - 1;           // first char of the lexeme

        if (isalpha(c) || c == '_') {              // identifier / keyword
            while (c != EOI && (isalnum(c) || c == '_')) c = tb.nextChar();
            if (c != EOI) tb.retract();
            string lex = tb.getLexeme();
            emit(KEYWORDS.count(lex) ? "KEYWORD" : "IDENTIFIER", lex);
        } else if (isdigit(c)) {                   // number
            while (c != EOI && isdigit(c)) c = tb.nextChar();
            if (c == '.') {
                c = tb.nextChar();
                while (c != EOI && isdigit(c)) c = tb.nextChar();
            }
            if (c != EOI) tb.retract();
            emit("NUMBER", tb.getLexeme());
        } else {                                   // operator / punctuation / comment
            int first = c;
            c = tb.nextChar();
            if (first == '/' && c == '/') {        // line comment
                while (c != EOI && c != '\n') c = tb.nextChar();
                continue;
            }
            if (first == '/' && c == '*') {        // block comment
                int prev = 0;
                c = tb.nextChar();
                while (c != EOI && !(prev == '*' && c == '/')) {
                    if (c == '\n') lineNo++;
                    prev = c; c = tb.nextChar();
                }
                c = tb.nextChar();
                continue;
            }
            bool two = (c == '=' && strchr("=!<>", first)) ||
                       (first == '&' && c == '&') || (first == '|' && c == '|');
            if (!two && c != EOI) tb.retract();
            emit(strchr("(){}[];,", first) ? "DELIMITER" : "OPERATOR", tb.getLexeme());
        }
        c = tb.nextChar();
    }
    cout << string(34, '-') << "\nEnd of input reached. Buffer reloads: " << tb.reloads << "\n";
}
