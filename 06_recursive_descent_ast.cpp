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
 * HOW IT WORKS
 *   - One member function per non-terminal (E, Ep, T, Tp, F).
 *   - Ep(left) / Tp(left) receive the tree built so far and extend it, which
 *     keeps + - * / LEFT associative although the grammar is right-recursive.
 *   - Evaluation walks the AST. Identifier values come from a symbol table
 *     (a=10 b=5 c=2 x=7 y=3); an unknown identifier is asked from the user.
 *
 * Build : g++ -std=c++17 -Wall -o rdp 06_recursive_descent_ast.cpp
 * Run   : ./rdp                       (the two required expressions)
 *         ./rdp "a * (b + 3)"         (your own expression)
 */
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <stdexcept>
#include <cctype>
using namespace std;

// ---------------- AST ----------------
struct Node;
using NodePtr = unique_ptr<Node>;
struct Node {
    enum Kind { NUM, VAR, OP } kind;
    char op = 0;
    double num = 0;
    string name;
    NodePtr l, r;
};
static NodePtr makeOp(char op, NodePtr l, NodePtr r) {
    auto n = make_unique<Node>();
    n->kind = Node::OP; n->op = op; n->l = move(l); n->r = move(r);
    return n;
}

// ---------------- parser ----------------
class Parser {
    enum Tok { ID, NUMBER, PLUS, MINUS, MUL, DIV, LP, RP, END, BAD };
    string in;
    size_t pos = 0;
    Tok look = END;
    string text;                        // lexeme of ID / NUMBER

    void advance() {
        while (pos < in.size() && isspace((unsigned char)in[pos])) pos++;
        if (pos >= in.size()) { look = END; return; }
        char c = in[pos];
        if (isalpha((unsigned char)c) || c == '_') {
            size_t s = pos;
            while (pos < in.size() && (isalnum((unsigned char)in[pos]) || in[pos] == '_')) pos++;
            text = in.substr(s, pos - s); look = ID; return;
        }
        if (isdigit((unsigned char)c) || c == '.') {
            size_t s = pos;
            while (pos < in.size() && (isdigit((unsigned char)in[pos]) || in[pos] == '.')) pos++;
            text = in.substr(s, pos - s); look = NUMBER; return;
        }
        pos++;
        switch (c) {
            case '+': look = PLUS;  break;  case '-': look = MINUS; break;
            case '*': look = MUL;   break;  case '/': look = DIV;   break;
            case '(': look = LP;    break;  case ')': look = RP;    break;
            default:  look = BAD;
        }
    }
    [[noreturn]] void error(const string &msg) {
        throw runtime_error("Syntax error: " + msg + " (near \"" + in.substr(pos) + "\")");
    }

    // E -> T E'
    NodePtr E() { return Ep(T()); }

    // E' -> + T E' | - T E' | epsilon
    NodePtr Ep(NodePtr left) {
        if (look == PLUS || look == MINUS) {
            char op = look == PLUS ? '+' : '-';
            advance();
            NodePtr right = T();
            return Ep(makeOp(op, move(left), move(right)));
        }
        return left;                                       // epsilon
    }

    // T -> F T'
    NodePtr T() { return Tp(F()); }

    // T' -> * F T' | / F T' | epsilon
    NodePtr Tp(NodePtr left) {
        if (look == MUL || look == DIV) {
            char op = look == MUL ? '*' : '/';
            advance();
            NodePtr right = F();
            return Tp(makeOp(op, move(left), move(right)));
        }
        return left;                                       // epsilon
    }

    // F -> ( E ) | id | num
    NodePtr F() {
        if (look == LP) {
            advance();
            NodePtr e = E();
            if (look != RP) error("expected ')'");
            advance();
            return e;
        }
        if (look == ID) {
            auto n = make_unique<Node>();
            n->kind = Node::VAR; n->name = text;
            advance();
            return n;
        }
        if (look == NUMBER) {
            auto n = make_unique<Node>();
            n->kind = Node::NUM; n->num = stod(text);
            advance();
            return n;
        }
        error("expected id, num or '('");
    }

public:
    NodePtr parse(const string &s) {
        in = s; pos = 0; advance();
        NodePtr root = E();
        if (look != END) error("unexpected token after expression");
        return root;
    }
};

// ---------------- evaluation ----------------
static map<string, double> symbols = {{"a", 10}, {"b", 5}, {"c", 2}, {"x", 7}, {"y", 3}};

static double lookup(const string &name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) return it->second;
    double v = 0;
    cout << "  value of '" << name << "' = ";
    cin >> v;
    return symbols[name] = v;
}

static double eval(const Node &n) {
    if (n.kind == Node::NUM) return n.num;
    if (n.kind == Node::VAR) return lookup(n.name);
    double a = eval(*n.l), b = eval(*n.r);
    switch (n.op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        default:
            if (b == 0) throw runtime_error("Runtime error: division by zero");
            return a / b;
    }
}

// ---------------- printing ----------------
static void printTree(const Node &n, const string &prefix, bool last) {
    cout << prefix << (last ? "`-- " : "|-- ");
    if (n.kind == Node::NUM) cout << "num(" << n.num << ")\n";
    else if (n.kind == Node::VAR) cout << "id(" << n.name << ")\n";
    else cout << n.op << "\n";
    if (n.kind != Node::OP) return;
    string next = prefix + (last ? "    " : "|   ");
    printTree(*n.l, next, false);
    printTree(*n.r, next, true);
}
static void infix(const Node &n) {
    if (n.kind == Node::NUM) cout << n.num;
    else if (n.kind == Node::VAR) cout << n.name;
    else { cout << "("; infix(*n.l); cout << " " << n.op << " "; infix(*n.r); cout << ")"; }
}
static void postfix(const Node &n) {
    if (n.kind == Node::OP) { postfix(*n.l); postfix(*n.r); cout << n.op << " "; }
    else if (n.kind == Node::NUM) cout << n.num << " ";
    else cout << n.name << " ";
}

static void run(const string &label, const string &expr) {
    cout << "\n==== " << label << " : " << expr << " ====\n";
    Parser p;
    NodePtr root = p.parse(expr);
    cout << "AST:\n";
    printTree(*root, "", true);
    cout << "Fully parenthesised : "; infix(*root);   cout << "\n";
    cout << "Postfix             : "; postfix(*root); cout << "\n";
    cout << "Value               : " << eval(*root) << "\n";
}

int main(int argc, char *argv[]) {
    cout << "Symbol table: a=10 b=5 c=2 x=7 y=3\n";
    try {
        if (argc > 1) { run("user expression", argv[1]); return 0; }
        run("Expression 1  (id + id * num)", "a + b * 4");
        run("Expression 2  ((id - num) / id)", "(a - 4) / b");
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return 1;
    }
}
