/*
 * QUESTION:
 * 1) To design and implement a syntax checker and evaluator for a basic
 *    configuration language or subset of a scripting language that supports
 *    variable assignments and dynamic variable evaluation using the Bison
 *    parser generator and Flex lexical analyzer.
 *
 * FILE: config.y  (Bison grammar + evaluator, C++ code)
 *
 * LANGUAGE
 *     program   : statement*
 *     statement : IDENT '=' expr ';'        assignment
 *               | 'print' expr ';'          show a value
 *     expr      : NUMBER | IDENT | expr (+ - * / %) expr | '(' expr ')' | '-' expr
 *   '#' starts a comment. A variable is evaluated when it is used (dynamic
 *   evaluation), so later assignments see earlier values.
 *
 * Build : bison -d -o config.tab.cc config.y
 *         flex -o lex.yy.cc config.l
 *         g++ -std=c++17 config.tab.cc lex.yy.cc -o config
 * Run   : ./config sample_ok.cfg
 *         ./config sample_err.cfg      (shows error recovery with line numbers)
 */
%{
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <map>
#include <string>
#include <vector>
using namespace std;

int yylex(void);
void yyerror(const char *msg);
extern int yylineno;
extern FILE *yyin;

static map<string, double> vars;
static vector<string> varOrder;          // to print the table in assignment order
static int errors = 0, assignments = 0;

static void setVar(const string &n, double v) {
    if (!vars.count(n)) varOrder.push_back(n);
    vars[n] = v;
    assignments++;
}
static double getVar(const string &n) {
    auto it = vars.find(n);
    if (it == vars.end()) {
        cerr << "Semantic error (line " << yylineno << "): variable '" << n << "' used before assignment\n";
        errors++;
        return 0;
    }
    return it->second;
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

%define parse.error verbose

%%
program
    : /* empty */
    | program statement
    ;

statement
    : IDENT '=' expr ';'      { setVar($1, $3); cout << "  " << $1 << " = " << $3 << "\n"; free($1); }
    | PRINT expr ';'          { cout << "  >> " << $2 << "\n"; }
    | error ';'               { yyerrok; /* skip to the next ';' and carry on */ }
    ;

expr
    : NUMBER                  { $$ = $1; }
    | IDENT                   { $$ = getVar($1); free($1); }
    | expr '+' expr           { $$ = $1 + $3; }
    | expr '-' expr           { $$ = $1 - $3; }
    | expr '*' expr           { $$ = $1 * $3; }
    | expr '/' expr           {
          if ($3 == 0) { cerr << "Semantic error (line " << yylineno << "): division by zero\n"; errors++; $$ = 0; }
          else $$ = $1 / $3;
      }
    | expr '%' expr           {
          if ($3 == 0) { cerr << "Semantic error (line " << yylineno << "): modulo by zero\n"; errors++; $$ = 0; }
          else $$ = fmod($1, $3);
      }
    | '-' expr %prec UMINUS   { $$ = -$2; }
    | '(' expr ')'            { $$ = $2; }
    ;
%%

void yyerror(const char *msg) {
    cerr << "Syntax error (line " << yylineno << "): " << msg << "\n";
    errors++;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) { perror(argv[1]); return 1; }
    }
    yyparse();
    cout << "\n---- Summary ----\n"
         << "Assignments executed : " << assignments << "\n"
         << "Errors found         : " << errors << "  => "
         << (errors ? "SYNTAX/SEMANTIC CHECK FAILED" : "syntax check PASSED") << "\n"
         << "Variable table:\n";
    for (auto &n : varOrder) cout << "  " << n << " = " << vars[n] << "\n";
    return errors ? 1 : 0;
}
