/*
 * QUESTION:
 * 2) Design and implement a syntax analyzer and semantic validator that
 *    processes and evaluates nested parentheses balance and mathematical
 *    structural formatting using the Bison parser generator and Flex lexical
 *    analyzer.
 *
 * FILE: paren.y  (Bison grammar + semantic validator, C++ code)
 *
 * WHAT IS CHECKED (one expression per input line)
 *   1. Parentheses balance - no unclosed '(' and no extra ')'  (scanner counters)
 *   2. Structural format   - operands/operators alternate, no empty "()",
 *                            no "2(3)", no "3 + * 4" (enforced by the grammar)
 *   3. Semantic validation - division by zero is reported
 *   4. Evaluation          - valid lines are evaluated; max nesting depth shown
 *
 * Build : bison -d -o paren.tab.cc paren.y
 *         flex -o lex.yy.cc paren.l
 *         g++ -std=c++17 paren.tab.cc lex.yy.cc -o paren
 * Run   : ./paren < sample_input.txt      or just ./paren and type lines
 */
%{
#include <iostream>
#include <cmath>
using namespace std;

int yylex(void);
void yyerror(const char *msg);
extern int cur_depth, max_depth, extra_close;

static int line_no = 1, good = 0, bad = 0, semErr = 0;

static void resetLine() { cur_depth = max_depth = extra_close = 0; semErr = 0; }

static void balanceReport() {
    if (cur_depth > 0)   cout << "   balance : UNBALANCED - " << cur_depth << " unclosed '('\n";
    if (extra_close > 0) cout << "   balance : UNBALANCED - " << extra_close << " extra ')'\n";
}
%}

%define api.value.type {double}
%token NUMBER
%left '+' '-'
%left '*' '/'
%right UMINUS
%right '^'

%%
input
    : /* empty */
    | input line
    ;

line
    : '\n'                    { line_no++; resetLine(); }
    | expr '\n'               {
          if (cur_depth == 0 && extra_close == 0 && !semErr) {
              cout << "Line " << line_no << ": VALID   balanced, max nesting depth = "
                   << max_depth << ", value = " << $1 << "\n";
              good++;
          } else {
              cout << "Line " << line_no << ": INVALID (semantic error)\n";
              bad++;
          }
          line_no++; resetLine();
      }
    | error '\n'              {
          cout << "Line " << line_no << ": INVALID (structure / balance error)\n";
          balanceReport();
          bad++; line_no++; resetLine(); yyerrok;
      }
    ;

expr
    : NUMBER                  { $$ = $1; }
    | expr '+' expr           { $$ = $1 + $3; }
    | expr '-' expr           { $$ = $1 - $3; }
    | expr '*' expr           { $$ = $1 * $3; }
    | expr '/' expr           {
          if ($3 == 0) { cout << "   semantic: division by zero\n"; semErr = 1; $$ = 0; }
          else $$ = $1 / $3;
      }
    | expr '^' expr           { $$ = pow($1, $3); }
    | '-' expr %prec UMINUS   { $$ = -$2; }
    | '(' expr ')'            { $$ = $2; }
    ;
%%

void yyerror(const char *msg) {
    cout << "   syntax  : " << msg << "\n";
}

int main() {
    cout << "Enter one arithmetic expression per line (Ctrl-D / EOF to finish)\n\n";
    yyparse();
    cout << "\nSummary: " << good << " valid, " << bad << " invalid expression(s)\n";
    return bad ? 1 : 0;
}
