/*
 * QUESTION:
 * 1) To design and implement a syntax checker and evaluator for a basic
 *    configuration language or subset of a scripting language that supports
 *    variable assignments and dynamic variable evaluation using the Bison
 *    parser generator and Flex lexical analyzer.
 *
 * FILE: config.l  (Flex scanner) - used together with config.y
 */
%option noyywrap yylineno nounput noinput
%{
#include <cstdlib>
#include <cstring>
#include "config.tab.hh"
%}

%%
[ \t\r\n]+                 { /* skip whitespace */ }
"#".*                      { /* skip comments */ }
"print"                    { return PRINT; }
[0-9]+(\.[0-9]+)?          { yylval.num = atof(yytext); return NUMBER; }
[A-Za-z_][A-Za-z0-9_]*     { yylval.str = strdup(yytext); return IDENT; }
.                          { return yytext[0]; }
%%
