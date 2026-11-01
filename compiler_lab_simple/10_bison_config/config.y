/*
 * QUESTION:
 * 1) To design and implement a syntax checker and evaluator for a basic
 *    configuration language or subset of a scripting language that supports
 *    variable assignments and dynamic variable evaluation using the Bison
 *    parser generator and Flex lexical analyzer.
 *
 * FILE: config.y  (Bison grammar + evaluator)
 *
 * LANGUAGE
 *     name = expression;        assignment
 *     print expression;         show a value
 *   expressions use numbers, variables, + - * / % and ( ).  '#' = comment.
 *   A variable is looked up when it is used (dynamic evaluation), so later
 *   assignments see the current values.
 *
 * Build : bison -d -o config.tab.cc config.y
 *         flex -o lex.yy.cc config.l
 *         g++ config.tab.cc lex.yy.cc -o config
 * Run   : ./config sample_ok.cfg
 *         ./config sample_err.cfg        (shows errors with line numbers)
 */
%{
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <map>
#include <string>
using namespace std;

int yylex();
void yyerror(const char *msg);
extern int yylineno;
extern FILE *yyin;

map<string, double> vars;      // the variable table
int errors = 0;

double getVar(string name) {
    if (!vars.count(name)) {
        cout << "Semantic error (line " << yylineno << "): '" << name << "' used before assignment\n";
        errors++;
        return 0;
    }
    return vars[name];
}
%}

%union { double num; char *str; }
%token <num> NUMBER
%token <str> IDENT
%token PRINT
%type  <num> expr

%left '+' '-'
%left '*' '/' '%'
%right UMINUS

%%
program : /* empty */
        | program statement
        ;

statement : IDENT '=' expr ';'   { vars[$1] = $3; cout << "  " << $1 << " = " << $3 << "\n"; }
          | PRINT expr ';'       { cout << "  >> " << $2 << "\n"; }
          | error ';'            { yyerrok; }          /* skip to the next ';' and go on */
          ;

expr : NUMBER                    { $$ = $1; }
     | IDENT                     { $$ = getVar($1); }
     | expr '+' expr             { $$ = $1 + $3; }
     | expr '-' expr             { $$ = $1 - $3; }
     | expr '*' expr             { $$ = $1 * $3; }
     | expr '/' expr             { if ($3 == 0) { cout << "Semantic error (line " << yylineno << "): division by zero\n"; errors++; $$ = 0; }
                                   else $$ = $1 / $3; }
     | expr '%' expr             { if ($3 == 0) { cout << "Semantic error (line " << yylineno << "): modulo by zero\n"; errors++; $$ = 0; }
                                   else $$ = fmod($1, $3); }
     | '-' expr %prec UMINUS     { $$ = -$2; }
     | '(' expr ')'              { $$ = $2; }
     ;
%%

void yyerror(const char *msg) {
    cout << "Syntax error (line " << yylineno << "): " << msg << "\n";
    errors++;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) { cout << "cannot open file\n"; return 1; }
    }
    yyparse();
    cout << "\nErrors found: " << errors << (errors ? "  -> CHECK FAILED\n" : "  -> syntax check PASSED\n");
    cout << "Variables:\n";
    for (map<string, double>::iterator it = vars.begin(); it != vars.end(); ++it)
        cout << "  " << it->first << " = " << it->second << "\n";
}
