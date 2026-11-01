/*
 * QUESTION:
 * 2) Design and implement a syntax analyzer and semantic validator that
 *    processes and evaluates nested parentheses balance and mathematical
 *    structural formatting using the Bison parser generator and Flex lexical
 *    analyzer.
 *
 * FILE: paren.l  (Flex scanner, C++ code) - works together with paren.y
 * The scanner also counts parentheses: current depth, maximum depth and the
 * number of unmatched ')' on the current line. The parser reads these
 * numbers when it reaches the end of the line.
 */
%option noyywrap yylineno nounput noinput
%{
#include <cstdlib>
#include "paren.tab.hh"

int cur_depth = 0;      /* currently open '(' on this line   */
int max_depth = 0;      /* deepest nesting seen on this line */
int extra_close = 0;    /* ')' without a matching '('        */
%}

%%
[ \t\r]+                   { /* ignore blanks */ }
[0-9]+(\.[0-9]+)?          { yylval = atof(yytext); return NUMBER; }
"("                        { cur_depth++; if (cur_depth > max_depth) max_depth = cur_depth; return '('; }
")"                        { if (cur_depth > 0) cur_depth--; else extra_close++; return ')'; }
[-+*/^]                    { return yytext[0]; }
\n                         { return '\n'; }
.                          { return yytext[0]; /* illegal symbol -> syntax error */ }
%%
