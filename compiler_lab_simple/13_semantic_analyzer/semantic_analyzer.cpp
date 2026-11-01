/*
 * QUESTION:
 * Write a program to implement a semantic analyzer that performs type
 * checking, and semantic error detection on an Abstract Syntax Tree (AST)
 * produced by a parser.
 *
 * PIPELINE:  source -> lexer -> parser -> AST -> semantic analyzer -> report
 *
 * Mini language: types int, float, bool, string; declarations with optional
 * initial value; assignments; if / else; while; { blocks }; print(expr);
 * expressions with + - * / % < > <= >= == != && || ! and ( ).
 *
 * The analyzer walks the AST once with a stack of scopes (symbol tables),
 * writes the type of every expression into its node and reports
 *   ERRORS  : undeclared variable, redeclaration in the same scope,
 *             type mismatch (int -> float is allowed, float -> int is not),
 *             invalid operand types for an operator, non-bool condition,
 *             division / modulo by constant zero, '%' on non-int operands
 *   WARNINGS: variable used before initialization, variable never used
 *
 * Build : g++ -o semantic semantic_analyzer.cpp
 * Run   : ./semantic sample_valid.mc
 *         ./semantic sample_errors.mc
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
using namespace std;

// =====================================================================
//  LEXER
// =====================================================================
struct Token {
    char kind;          // 'i' identifier/keyword, 'n' int, 'f' float, 's' string, 'o' operator, 'e' end
    string text;
    int line;
};
vector<Token> tokens;
int p = 0;              // index of the current token

void addToken(char kind, string text, int line) {
    Token t;
    t.kind = kind; t.text = text; t.line = line;
    tokens.push_back(t);
}

void lexSource(string s) {
    int i = 0, line = 1, n = s.size();
    while (i < n) {
        char c = s[i];
        if (c == '\n') { line++; i++; }
        else if (isspace(c)) i++;
        else if (c == '/' && i + 1 < n && s[i + 1] == '/') { while (i < n && s[i] != '\n') i++; }
        else if (isalpha(c) || c == '_') {
            int st = i;
            while (i < n && (isalnum(s[i]) || s[i] == '_')) i++;
            addToken('i', s.substr(st, i - st), line);
        }
        else if (isdigit(c)) {
            int st = i;
            char kind = 'n';
            while (i < n && isdigit(s[i])) i++;
            if (i + 1 < n && s[i] == '.' && isdigit(s[i + 1])) {
                kind = 'f';
                i++;
                while (i < n && isdigit(s[i])) i++;
            }
            addToken(kind, s.substr(st, i - st), line);
        }
        else if (c == '"') {
            int st = ++i;
            while (i < n && s[i] != '"' && s[i] != '\n') i++;
            if (i >= n || s[i] != '"') throw string("Line " + to_string(line) + ": unterminated string");
            addToken('s', s.substr(st, i - st), line);
            i++;
        }
        else {
            string two = s.substr(i, 2);
            if (two == "==" || two == "!=" || two == "<=" || two == ">=" || two == "&&" || two == "||") {
                addToken('o', two, line);
                i += 2;
            } else if (strchr("+-*/%<>!=(){};", c)) {
                addToken('o', string(1, c), line);
                i++;
            } else throw string("Line " + to_string(line) + ": illegal character '" + string(1, c) + "'");
        }
    }
    addToken('e', "<end>", line);
}

// =====================================================================
//  AST
// =====================================================================
struct Node {
    string kind;        // Program Block Decl Assign If While Print Binary Unary Ident IntLit FloatLit BoolLit StrLit
    string text;        // name / operator / literal
    string type;        // Decl: declared type;  expressions: type found by the analyzer
    int line;
    vector<Node *> kids;
};

Node *mk(string kind, int line, string text = "") {
    Node *n = new Node;
    n->kind = kind; n->line = line; n->text = text; n->type = "";
    return n;
}

// =====================================================================
//  PARSER (recursive descent)
// =====================================================================
void fail(string msg) {
    throw string("Line " + to_string(tokens[p].line) + ": syntax error: " + msg + " (found '" + tokens[p].text + "')");
}
bool is(string t) { return (tokens[p].kind == 'i' || tokens[p].kind == 'o') && tokens[p].text == t; }
bool accept(string t) { if (is(t)) { p++; return true; } return false; }
void expect(string t) { if (!accept(t)) fail("expected '" + t + "'"); }
bool isTypeWord(string s) { return s == "int" || s == "float" || s == "bool" || s == "string"; }

Node *expr();

Node *primary() {
    int line = tokens[p].line;
    Token t = tokens[p];
    if (t.kind == 'n') { p++; return mk("IntLit", line, t.text); }
    if (t.kind == 'f') { p++; return mk("FloatLit", line, t.text); }
    if (t.kind == 's') { p++; return mk("StrLit", line, t.text); }
    if (accept("true"))  return mk("BoolLit", line, "true");
    if (accept("false")) return mk("BoolLit", line, "false");
    if (accept("(")) { Node *e = expr(); expect(")"); return e; }
    if (t.kind == 'i' && !isTypeWord(t.text) && t.text != "if" && t.text != "while" && t.text != "print") {
        p++;
        return mk("Ident", line, t.text);
    }
    fail("expected an expression");
    return NULL;
}

Node *unary() {
    if (is("-") || is("!")) {
        Node *u = mk("Unary", tokens[p].line, tokens[p].text);
        p++;
        u->kids.push_back(unary());
        return u;
    }
    return primary();
}

// operator precedence levels, lowest first
const char *levels[6][5] = {{"||"}, {"&&"}, {"==", "!="}, {"<", ">", "<=", ">="}, {"+", "-"}, {"*", "/", "%"}};

Node *binary(int level) {
    if (level == 6) return unary();
    Node *left = binary(level + 1);
    while (true) {
        string op = "";
        if (tokens[p].kind == 'o')
            for (int k = 0; k < 5 && levels[level][k]; k++)
                if (tokens[p].text == levels[level][k]) op = tokens[p].text;
        if (op == "") break;
        Node *b = mk("Binary", tokens[p].line, op);
        p++;
        b->kids.push_back(left);
        b->kids.push_back(binary(level + 1));
        left = b;
    }
    return left;
}

Node *expr() { return binary(0); }

Node *statement() {
    int line = tokens[p].line;
    if (accept("{")) {
        Node *b = mk("Block", line);
        while (!is("}")) {
            if (tokens[p].kind == 'e') fail("missing '}'");
            b->kids.push_back(statement());
        }
        expect("}");
        return b;
    }
    if (accept("if")) {
        Node *n = mk("If", line);
        expect("("); n->kids.push_back(expr()); expect(")");
        n->kids.push_back(statement());
        if (accept("else")) n->kids.push_back(statement());
        return n;
    }
    if (accept("while")) {
        Node *n = mk("While", line);
        expect("("); n->kids.push_back(expr()); expect(")");
        n->kids.push_back(statement());
        return n;
    }
    if (accept("print")) {
        Node *n = mk("Print", line);
        expect("("); n->kids.push_back(expr()); expect(")"); expect(";");
        return n;
    }
    if (tokens[p].kind == 'i' && isTypeWord(tokens[p].text)) {      // declaration
        Node *d = mk("Decl", line);
        d->type = tokens[p++].text;
        if (tokens[p].kind != 'i') fail("expected a variable name");
        d->text = tokens[p++].text;
        if (accept("=")) d->kids.push_back(expr());
        expect(";");
        return d;
    }
    if (tokens[p].kind == 'i' && tokens[p + 1].kind == 'o' && tokens[p + 1].text == "=") {   // assignment
        Node *a = mk("Assign", line, tokens[p].text);
        p += 2;
        a->kids.push_back(expr());
        expect(";");
        return a;
    }
    fail("expected a statement");
    return NULL;
}

// =====================================================================
//  SEMANTIC ANALYZER
// =====================================================================
struct Symbol {
    string type;
    bool initialized, used;
    int line;
};
vector<map<string, Symbol> > scopes;                  // stack of symbol tables
vector<pair<int, string> > errors, warnings;
vector<string> symbolLog;                             // for the symbol table printout

void error(int line, string msg)   { errors.push_back(make_pair(line, msg)); }
void warning(int line, string msg) { warnings.push_back(make_pair(line, msg)); }

void pushScope() { scopes.push_back(map<string, Symbol>()); }

void popScope() {
    int level = scopes.size() - 1;
    for (map<string, Symbol>::iterator it = scopes.back().begin(); it != scopes.back().end(); ++it) {
        symbolLog.push_back(it->first + "  " + it->second.type + "  scope " + to_string(level) +
                            "  line " + to_string(it->second.line) + (it->second.used ? "  used" : "  unused"));
        if (!it->second.used) warning(it->second.line, "variable '" + it->first + "' is declared but never used");
    }
    scopes.pop_back();
}

Symbol *lookup(string name) {                         // search from the innermost scope outwards
    for (int i = scopes.size() - 1; i >= 0; i--)
        if (scopes[i].count(name)) return &scopes[i][name];
    return NULL;
}

bool isNumeric(string t) { return t == "int" || t == "float"; }

// can a value of type 'src' be stored in a variable of type 'dst' ?
void checkAssignable(string dst, string src, int line, string where) {
    if (dst == "error" || src == "error" || dst == src) return;
    if (dst == "float" && src == "int") return;       // widening is fine
    if (dst == "int" && src == "float")
        error(line, where + ": cannot convert 'float' to 'int' (possible loss of precision)");
    else
        error(line, where + ": type mismatch, cannot convert '" + src + "' to '" + dst + "'");
}

string check(Node *n);

string checkBinary(Node *n) {
    string l = check(n->kids[0]), r = check(n->kids[1]);
    if (l == "error" || r == "error") return "error";
    string op = n->text;
    bool numeric = isNumeric(l) && isNumeric(r);
    string bad = "operator '" + op + "' cannot be applied to '" + l + "' and '" + r + "'";

    if (op == "+" || op == "-" || op == "*" || op == "/") {
        Node *right = n->kids[1];
        if (op == "/" && (right->kind == "IntLit" || right->kind == "FloatLit") && atof(right->text.c_str()) == 0) {
            error(n->line, "division by zero");
            return "error";
        }
        if (numeric) return (l == "float" || r == "float") ? "float" : "int";
        if (op == "+" && l == "string" && r == "string") return "string";
        error(n->line, bad);
        return "error";
    }
    if (op == "%") {
        if (l == "int" && r == "int") return "int";
        error(n->line, "operator '%' needs int operands, found '" + l + "' and '" + r + "'");
        return "error";
    }
    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (numeric) return "bool";
        error(n->line, bad);
        return "error";
    }
    if (op == "==" || op == "!=") {
        if (numeric || l == r) return "bool";
        error(n->line, bad);
        return "error";
    }
    if (l == "bool" && r == "bool") return "bool";    // && ||
    error(n->line, bad);
    return "error";
}

string checkExpr(Node *n) {
    if (n->kind == "IntLit") return "int";
    if (n->kind == "FloatLit") return "float";
    if (n->kind == "BoolLit") return "bool";
    if (n->kind == "StrLit") return "string";
    if (n->kind == "Ident") {
        Symbol *s = lookup(n->text);
        if (s == NULL) { error(n->line, "undeclared variable '" + n->text + "'"); return "error"; }
        s->used = true;
        if (!s->initialized) {
            warning(n->line, "variable '" + n->text + "' may be used before initialization");
            s->initialized = true;                    // warn only once
        }
        return s->type;
    }
    if (n->kind == "Unary") {
        string t = check(n->kids[0]);
        if (t == "error") return "error";
        if (n->text == "-") {
            if (isNumeric(t)) return t;
            error(n->line, "unary '-' cannot be applied to '" + t + "'");
        } else {
            if (t == "bool") return "bool";
            error(n->line, "'!' needs a bool operand, found '" + t + "'");
        }
        return "error";
    }
    return checkBinary(n);
}

string check(Node *n) {                               // compute the type and store it in the node
    n->type = checkExpr(n);
    return n->type;
}

void checkCondition(Node *cond, string what) {
    string t = check(cond);
    if (t != "error" && t != "bool")
        error(cond->line, "condition of '" + what + "' must be bool, found '" + t + "'");
}

void analyze(Node *n) {
    if (n->kind == "Block") {
        pushScope();
        for (int i = 0; i < (int)n->kids.size(); i++) analyze(n->kids[i]);
        popScope();
    }
    else if (n->kind == "Decl") {
        string init = "";
        if (!n->kids.empty()) init = check(n->kids[0]);          // checked before the name becomes visible
        if (scopes.back().count(n->text))
            error(n->line, "redeclaration of '" + n->text + "' (first declared at line " +
                           to_string(scopes.back()[n->text].line) + ")");
        else {
            Symbol s;
            s.type = n->type; s.initialized = !n->kids.empty(); s.used = false; s.line = n->line;
            scopes.back()[n->text] = s;
        }
        if (!n->kids.empty()) checkAssignable(n->type, init, n->line, "initialization of '" + n->text + "'");
    }
    else if (n->kind == "Assign") {
        string rt = check(n->kids[0]);
        Symbol *s = lookup(n->text);
        if (s == NULL) error(n->line, "assignment to undeclared variable '" + n->text + "'");
        else {
            n->type = s->type;
            checkAssignable(s->type, rt, n->line, "assignment to '" + n->text + "'");
            s->initialized = true;
        }
    }
    else if (n->kind == "If" || n->kind == "While") {
        checkCondition(n->kids[0], n->kind == "If" ? "if" : "while");
        for (int i = 1; i < (int)n->kids.size(); i++) {         // each branch has its own scope
            pushScope();
            analyze(n->kids[i]);
            popScope();
        }
    }
    else if (n->kind == "Print") check(n->kids[0]);
}

// =====================================================================
//  OUTPUT
// =====================================================================
void printAST(Node *n, int depth) {
    cout << string(depth * 3, ' ') << n->kind;
    if (n->text != "") cout << " " << n->text;
    if (n->type != "") cout << " : " << n->type;
    cout << "   [line " << n->line << "]\n";
    for (int i = 0; i < (int)n->kids.size(); i++) printAST(n->kids[i], depth + 1);
}

int main(int argc, char *argv[]) {
    if (argc < 2) { cout << "Usage: ./semantic <file.mc>\n"; return 1; }
    ifstream f(argv[1]);
    if (!f) { cout << "cannot open file\n"; return 1; }
    stringstream ss;
    ss << f.rdbuf();

    Node *program = mk("Program", 1);
    try {
        lexSource(ss.str());
        while (tokens[p].kind != 'e') program->kids.push_back(statement());
    } catch (string msg) {
        cout << "Syntax error - " << msg << "\n";
        return 2;
    }

    pushScope();                                       // global scope
    for (int i = 0; i < (int)program->kids.size(); i++) analyze(program->kids[i]);
    popScope();

    sort(errors.begin(), errors.end());
    sort(warnings.begin(), warnings.end());

    cout << "======== ANNOTATED AST ========\n";
    printAST(program, 0);
    cout << "\n======== SYMBOL TABLE (name type scope line status) ========\n";
    for (int i = 0; i < (int)symbolLog.size(); i++) cout << symbolLog[i] << "\n";
    cout << "\n======== SEMANTIC REPORT ========\n";
    for (int i = 0; i < (int)errors.size(); i++)   cout << "Line " << errors[i].first << ": error:   " << errors[i].second << "\n";
    for (int i = 0; i < (int)warnings.size(); i++) cout << "Line " << warnings[i].first << ": warning: " << warnings[i].second << "\n";
    cout << "\n" << errors.size() << " error(s), " << warnings.size() << " warning(s)  =>  "
         << (errors.empty() ? "SEMANTIC ANALYSIS PASSED" : "SEMANTIC ANALYSIS FAILED") << "\n";
    return errors.empty() ? 0 : 3;
}
