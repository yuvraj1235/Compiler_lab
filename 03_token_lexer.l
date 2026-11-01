/*
 * QUESTION:
 * 1) Write a Lex (or Flex) program that recognizes tokens for a small
 *    programming language fragment and prints each token along with its
 *    type and lexeme.
 *
 * Language fragment : keywords (int float char void if else while for
 *   return print), identifiers, integer/float constants, string literals,
 *   operators, delimiters, comments (// and block) and whitespace.
 *
 * The actions are written in C++; the generated scanner is compiled with g++.
 * Build : flex -o lex.yy.cc 03_token_lexer.l
 *         g++ -std=c++17 lex.yy.cc -o lexer
 * Run   : ./lexer sample_input.src      (or  echo "int a = 5;" | ./lexer)
 */
%option noyywrap yylineno nounput noinput
%x COMMENT
%{
#include <iostream>
#include <iomanip>
#include <cstdio>
using namespace std;

static void show(const char *type) {
    cout << left << setw(6) << yylineno << setw(14) << type << yytext << "\n";
}
%}

DIGIT   [0-9]
LETTER  [A-Za-z_]

%%
"//".*                          { /* single-line comment: discard */ }
"/*"                            { BEGIN(COMMENT); }
<COMMENT>"*/"                   { BEGIN(INITIAL); }
<COMMENT>.|\n                   { /* discard comment body */ }
<COMMENT><<EOF>>                { cout << "Lexical error: unterminated comment\n"; return 0; }

"int"|"float"|"char"|"void"|"if"|"else"|"while"|"for"|"return"|"print" { show("KEYWORD"); }
{LETTER}({LETTER}|{DIGIT})*     { show("IDENTIFIER"); }
{DIGIT}+\.{DIGIT}+              { show("FLOAT_CONST"); }
{DIGIT}+                        { show("INT_CONST"); }
\"[^\"\n]*\"                    { show("STRING"); }
"=="|"!="|"<="|">="|"&&"|"||"|"++"|"--"|"+="|"-="|"*="|"/=" { show("OPERATOR"); }
[-+*/%=<>!]                     { show("OPERATOR"); }
[(){}\[\];,]                    { show("DELIMITER"); }
[ \t\r\n]+                      { /* discard whitespace; yylineno counts lines */ }
.                               { cout << left << setw(6) << yylineno << setw(14) << "ILLEGAL"
                                       << yytext << "   <-- lexical error\n"; }
%%
int main(int argc, char *argv[]) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) { perror(argv[1]); return 1; }
    }
    cout << left << setw(6) << "LINE" << setw(14) << "TOKEN TYPE" << "LEXEME\n"
         << "------------------------------------\n";
    yylex();
    return 0;
}
