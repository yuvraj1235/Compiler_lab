/*
 * QUESTION:
 * 2) Design and implement a syntax analyzer and semantic validator that
 *    processes and evaluates nested parentheses balance and mathematical
 *    structural formatting using the Bison parser generator and Flex lexical
 *    analyzer.
 *
 * FILE: paren.y  (Bison grammar + validator)
 *
 * For every input line the program checks
 *   1. parentheses balance      (counters kept by the scanner)
 *   2. structure of the expression (the grammar rejects "()", "3 + * 4", "2(3)")
 *   3. semantics: division by zero
 * and prints the maximum nesting depth and the value of valid lines.
 *
 * Build : bison -d -o paren.tab.cc paren.y
 *         flex -o lex.yy.cc paren.l
 *         g++ paren.tab.cc lex.yy.cc -o paren
 * Run   : ./paren < sample_input.txt      (or run ./paren and type lines)
 */
%{
#include <iostream>
#include <cmath>
using namespace std;

int yylex();
void yyerror(const char *msg);
extern int depth, maxDepth, extraClose;

int lineNo = 1, good = 0, bad = 0, divZero = 0;

void newLine() { lineNo++; depth = maxDepth = extraClose = divZero = 0; }
%}

%define api.value.type {double}
%token NUMBER
%left '+' '-'
%left '*' '/'
%right UMINUS
%right '^'

%%
input : /* empty */
      | input line
      ;

line : '\n'               { newLine(); }
     | expr '\n'          { if (depth == 0 && extraClose == 0 && !divZero) {
                                cout << "Line " << lineNo << ": VALID   max depth = " << maxDepth << ", value = " << $1 << "\n";
                                good++;
                            } else {
                                cout << "Line " << lineNo << ": INVALID (semantic error)\n";
                                bad++;
                            }
                            newLine(); }
     | error '\n'         { cout << "Line " << lineNo << ": INVALID";
                            if (depth > 0) cout << "  (" << depth << " unclosed '(')";
                            if (extraClose > 0) cout << "  (" << extraClose << " extra ')')";
                            cout << "\n";
                            bad++; newLine(); yyerrok; }
     ;

expr : NUMBER             { $$ = $1; }
     | expr '+' expr      { $$ = $1 + $3; }
     | expr '-' expr      { $$ = $1 - $3; }
     | expr '*' expr      { $$ = $1 * $3; }
     | expr '/' expr      { if ($3 == 0) { cout << "   semantic: division by zero\n"; divZero = 1; $$ = 0; }
                            else $$ = $1 / $3; }
     | expr '^' expr      { $$ = pow($1, $3); }
     | '-' expr %prec UMINUS   { $$ = -$2; }
     | '(' expr ')'       { $$ = $2; }
     ;
%%

void yyerror(const char *msg) {
    cout << "   syntax: " << msg << "\n";
}

int main() {
    yyparse();
    cout << "\nSummary: " << good << " valid, " << bad << " invalid\n";
}
