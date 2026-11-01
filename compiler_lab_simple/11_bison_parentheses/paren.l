/*
 * QUESTION:
 * 2) Design and implement a syntax analyzer and semantic validator that
 *    processes and evaluates nested parentheses balance and mathematical
 *    structural formatting using the Bison parser generator and Flex lexical
 *    analyzer.
 *
 * FILE: paren.l  (Flex scanner) - used together with paren.y
 * The scanner also counts the parentheses of the current line.
 */
%option noyywrap nounput noinput
%{
#include <cstdlib>
#include "paren.tab.hh"

int depth = 0;          /* '(' currently open on this line   */
int maxDepth = 0;       /* deepest nesting on this line      */
int extraClose = 0;     /* ')' that has no matching '('      */
%}

%%
[ \t\r]+                   { /* ignore blanks */ }
[0-9]+(\.[0-9]+)?          { yylval = atof(yytext); return NUMBER; }
"("                        { depth++; if (depth > maxDepth) maxDepth = depth; return '('; }
")"                        { if (depth > 0) depth--; else extraClose++; return ')'; }
\n                         { return '\n'; }
.                          { return yytext[0]; }
%%
