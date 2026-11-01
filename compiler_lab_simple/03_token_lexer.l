/*
 * QUESTION:
 * 1) Write a Lex (or Flex) program that recognizes tokens for a small
 *    programming language fragment and prints each token along with its
 *    type and lexeme.
 *
 * Build : flex -o lex.yy.cc 03_token_lexer.l
 *         g++ lex.yy.cc -o lexer
 * Run   : ./lexer sample_input.src        (or: echo "int a = 5;" | ./lexer)
 */
%option noyywrap yylineno nounput noinput
%x COMMENT
%{
#include <iostream>
#include <iomanip>
#include <cstdio>
using namespace std;

void show(const char *type) {
    cout << left << setw(6) << yylineno << setw(14) << type << yytext << "\n";
}
%}

DIGIT   [0-9]
LETTER  [A-Za-z_]

%%
"//".*                          { /* line comment: ignore */ }
"/*"                            { BEGIN(COMMENT); }
<COMMENT>"*/"                   { BEGIN(INITIAL); }
<COMMENT>.|\n                   { /* inside comment: ignore */ }
<COMMENT><<EOF>>                { cout << "Lexical error: unterminated comment\n"; return 0; }

"int"|"float"|"char"|"void"|"if"|"else"|"while"|"for"|"return"|"print"  { show("KEYWORD"); }
{LETTER}({LETTER}|{DIGIT})*     { show("IDENTIFIER"); }
{DIGIT}+\.{DIGIT}+              { show("FLOAT_CONST"); }
{DIGIT}+                        { show("INT_CONST"); }
\"[^\"\n]*\"                    { show("STRING"); }
"=="|"!="|"<="|">="|"&&"|"||"   { show("OPERATOR"); }
[-+*/%=<>!]                     { show("OPERATOR"); }
[(){}\[\];,]                    { show("DELIMITER"); }
[ \t\r\n]+                      { /* whitespace: ignore */ }
.                               { cout << left << setw(6) << yylineno << setw(14) << "ILLEGAL"
                                       << yytext << "   <-- lexical error\n"; }
%%
int main(int argc, char *argv[]) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) { cout << "cannot open file\n"; return 1; }
    }
    cout << left << setw(6) << "LINE" << setw(14) << "TOKEN TYPE" << "LEXEME\n"
         << "------------------------------------\n";
    yylex();
}
