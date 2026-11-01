/*
 * QUESTION:
 * 1) Write a recursive-descent parser for the following grammar and build a
 *    basic abstract syntax tree (AST) and evaluate the expression
 *    id + id * num, (id - num) / id.
 *
 *    Grammar:
 *      E  -> T E'
 *      E' -> + T E' | - T E' | epsilon
 *      T  -> F T'
 *      T' -> * F T' | / F T' | epsilon
 *      F  -> ( E ) | id | num
 *
 * IDEA
 *   One function per non-terminal. Ep(left) and Tp(left) get the tree built
 *   so far and extend it, which keeps + - * / left associative.
 *   Ids get their values from a small table: a=10 b=5 c=2 x=7 y=3
 *   (an unknown id is asked from the user).
 *
 * Build : g++ -o rdp 06_recursive_descent_ast.cpp
 * Run   : ./rdp                     (the two required expressions)
 *         ./rdp "a * (b + 3)"       (your own expression)
 */
#include <iostream>
#include <string>
#include <map>
#include <cctype>
#include <cstdlib>
using namespace std;

// ---------- AST node ----------
struct Node {
    char type;              // 'n' number, 'v' variable, 'o' operator
    char op;                // + - * /
    double value;           // for numbers
    string name;            // for variables
    Node *left, *right;
};

Node *newNode(char type) {
    Node *n = new Node;
    n->type = type; n->op = 0; n->value = 0;
    n->left = n->right = NULL;
    return n;
}

Node *makeOp(char op, Node *l, Node *r) {
    Node *n = newNode('o');
    n->op = op; n->left = l; n->right = r;
    return n;
}

// ---------- lexer: look = 'i' id, 'n' number, '$' end, otherwise the char itself ----------
string input;
int pos = 0;
char look;
string word;

void advance() {
    while (pos < (int)input.size() && isspace(input[pos])) pos++;
    if (pos >= (int)input.size()) { look = '$'; return; }
    char c = input[pos];
    word = "";
    if (isalpha(c)) {
        while (pos < (int)input.size() && isalnum(input[pos])) word += input[pos++];
        look = 'i';
    } else if (isdigit(c)) {
        while (pos < (int)input.size() && (isdigit(input[pos]) || input[pos] == '.')) word += input[pos++];
        look = 'n';
    } else {
        look = c;
        pos++;
    }
}

void error(string msg) {
    cout << "Syntax error: " << msg << "\n";
    exit(1);
}

// ---------- parser (one function per non-terminal) ----------
Node *E();
Node *Ep(Node *left);
Node *T();
Node *Tp(Node *left);
Node *F();

Node *E() {                                  // E -> T E'
    Node *t = T();
    return Ep(t);
}

Node *Ep(Node *left) {                       // E' -> + T E' | - T E' | epsilon
    if (look == '+' || look == '-') {
        char op = look;
        advance();
        Node *right = T();
        return Ep(makeOp(op, left, right));
    }
    return left;
}

Node *T() {                                  // T -> F T'
    Node *f = F();
    return Tp(f);
}

Node *Tp(Node *left) {                       // T' -> * F T' | / F T' | epsilon
    if (look == '*' || look == '/') {
        char op = look;
        advance();
        Node *right = F();
        return Tp(makeOp(op, left, right));
    }
    return left;
}

Node *F() {                                  // F -> ( E ) | id | num
    if (look == '(') {
        advance();
        Node *e = E();
        if (look != ')') error("expected ')'");
        advance();
        return e;
    }
    if (look == 'i') {
        Node *n = newNode('v');
        n->name = word;
        advance();
        return n;
    }
    if (look == 'n') {
        Node *n = newNode('n');
        n->value = atof(word.c_str());
        advance();
        return n;
    }
    error("expected id, num or '('");
    return NULL;
}

// ---------- evaluation ----------
map<string, double> vars;

double eval(Node *n) {
    if (n->type == 'n') return n->value;
    if (n->type == 'v') {
        if (!vars.count(n->name)) {
            cout << "  value of " << n->name << " = ";
            cin >> vars[n->name];
        }
        return vars[n->name];
    }
    double a = eval(n->left), b = eval(n->right);
    if (n->op == '+') return a + b;
    if (n->op == '-') return a - b;
    if (n->op == '*') return a * b;
    if (b == 0) { cout << "Runtime error: division by zero\n"; exit(1); }
    return a / b;
}

// ---------- printing ----------
void printTree(Node *n, int depth) {         // pre-order, 4 spaces per level
    cout << string(depth * 4, ' ');
    if (n->type == 'n') cout << "num(" << n->value << ")\n";
    else if (n->type == 'v') cout << "id(" << n->name << ")\n";
    else {
        cout << n->op << "\n";
        printTree(n->left, depth + 1);
        printTree(n->right, depth + 1);
    }
}

void postfix(Node *n) {
    if (n->type == 'o') { postfix(n->left); postfix(n->right); cout << n->op << " "; }
    else if (n->type == 'n') cout << n->value << " ";
    else cout << n->name << " ";
}

void run(string label, string expr) {
    cout << "\n=== " << label << " : " << expr << " ===\n";
    input = expr;
    pos = 0;
    advance();
    Node *root = E();
    if (look != '$') error("unexpected token after the expression");
    cout << "AST:\n";
    printTree(root, 1);
    cout << "Postfix : ";
    postfix(root);
    cout << "\nValue   : " << eval(root) << "\n";
}

int main(int argc, char *argv[]) {
    vars["a"] = 10; vars["b"] = 5; vars["c"] = 2; vars["x"] = 7; vars["y"] = 3;
    cout << "Values: a=10 b=5 c=2 x=7 y=3\n";
    if (argc > 1) { run("your expression", argv[1]); return 0; }
    run("id + id * num", "a + b * 4");
    run("(id - num) / id", "(a - 4) / b");
}
