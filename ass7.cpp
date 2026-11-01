/*
 * QUESTION:
 * Write a program to implement a semantic analyzer that performs type
 * checking, and semantic error detection on an Abstract Syntax Tree (AST)
 * produced by a parser.
 *
 * OVERVIEW
 *   source file --> Lexer --> Parser --> AST --> SemanticAnalyzer --> report
 *
 *   Source language (mini-C): types int, float, bool, string, void;
 *   global variables, functions, blocks, if/else, while, break, continue,
 *   return, print(expr), assignments, calls and the usual operators.
 *
 *   The semantic analyzer walks the AST once (after a pre-pass that records
 *   all function signatures so functions may be called before they are
 *   defined) using a stack of scopes (symbol tables). It annotates every
 *   expression node with its type and detects:
 *
 *     ERRORS
 *       - undeclared variable / function, redeclaration in the same scope
 *       - type mismatch in assignment, initialisation, argument or return
 *         (int -> float widening is allowed; float -> int is an error)
 *       - invalid operand types for an operator (e.g. "a" - 1, !5, 1 && 2)
 *       - non-bool condition in if / while
 *       - wrong number / type of arguments in a call
 *       - calling a variable, or using a function name as a variable
 *       - division / modulo by constant zero, % on non-int operands
 *       - return value in a void function, missing return value,
 *         non-void function that can reach its end without returning
 *       - break / continue outside a loop
 *       - variable of type void, use of a void value
 *     WARNINGS
 *       - variable used before initialisation
 *       - variable declared but never used
 *       - expression statement with no effect, missing main()
 *
 * Build : g++ -std=c++17 -Wall -o semantic semantic_analyzer.cpp
 * Run   : ./semantic sample_valid.mc
 *         ./semantic sample_errors.mc
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <cctype>
using namespace std;

// ============================================================================
//  Types
// ============================================================================
enum class Type { Int, Float, Bool, String, Void, Error };

static string tname(Type t) {
    switch (t) {
        case Type::Int: return "int";    case Type::Float: return "float";
        case Type::Bool: return "bool";  case Type::String: return "string";
        case Type::Void: return "void";  default: return "<error>";
    }
}
static bool isNumeric(Type t) { return t == Type::Int || t == Type::Float; }

// ============================================================================
//  AST
// ============================================================================
enum class K { Program, FuncDecl, Param, VarDecl, Block, If, While, Return, Break,
               Continue, Print, ExprStmt, Assign, Binary, Unary, Call, Ident,
               IntLit, FloatLit, BoolLit, StrLit };

struct Node;
using NodePtr = unique_ptr<Node>;
struct Node {
    K kind;
    int line;
    string text;                       // name / operator / literal text
    Type declType = Type::Void;        // declared type (decls)
    Type type = Type::Void;            // computed type (filled by analyzer)
    vector<NodePtr> kids;
    Node(K k, int l, string t = "") : kind(k), line(l), text(move(t)) {}
};
static NodePtr mk(K k, int line, const string &text = "") { return make_unique<Node>(k, line, text); }

// ============================================================================
//  Lexer
// ============================================================================
struct Token {
    enum Kind { Id, IntL, FloatL, StrL, Op, End } kind;
    string text;
    int line;
};

static vector<Token> lexSource(const string &s) {
    vector<Token> out;
    size_t i = 0, n = s.size();
    int line = 1;
    auto at = [&](size_t k) { return k < n ? s[k] : '\0'; };
    while (i < n) {
        char c = s[i];
        if (c == '\n') { line++; i++; continue; }
        if (isspace((unsigned char)c)) { i++; continue; }
        if (c == '/' && at(i + 1) == '/') { while (i < n && s[i] != '\n') i++; continue; }
        if (c == '/' && at(i + 1) == '*') {
            int start = line;
            i += 2;
            while (i < n && !(s[i] == '*' && at(i + 1) == '/')) { if (s[i] == '\n') line++; i++; }
            if (i >= n) throw runtime_error("Line " + to_string(start) + ": unterminated comment");
            i += 2; continue;
        }
        if (isalpha((unsigned char)c) || c == '_') {
            size_t st = i;
            while (isalnum((unsigned char)at(i)) || at(i) == '_') i++;
            out.push_back({Token::Id, s.substr(st, i - st), line});
        } else if (isdigit((unsigned char)c)) {
            size_t st = i;
            while (isdigit((unsigned char)at(i))) i++;
            bool isF = false;
            if (at(i) == '.' && isdigit((unsigned char)at(i + 1))) {
                isF = true; i++;
                while (isdigit((unsigned char)at(i))) i++;
            }
            out.push_back({isF ? Token::FloatL : Token::IntL, s.substr(st, i - st), line});
        } else if (c == '"') {
            size_t st = ++i;
            while (i < n && s[i] != '"' && s[i] != '\n') i++;
            if (at(i) != '"') throw runtime_error("Line " + to_string(line) + ": unterminated string literal");
            out.push_back({Token::StrL, s.substr(st, i - st), line});
            i++;
        } else {
            string two = s.substr(i, 2);
            if (two == "==" || two == "!=" || two == "<=" || two == ">=" || two == "&&" || two == "||") {
                out.push_back({Token::Op, two, line}); i += 2;
            } else if (string("+-*/%<>!=(){};,").find(c) != string::npos) {
                out.push_back({Token::Op, string(1, c), line}); i++;
            } else {
                throw runtime_error("Line " + to_string(line) + ": illegal character '" + string(1, c) + "'");
            }
        }
    }
    out.push_back({Token::End, "<eof>", line});
    return out;
}

// ============================================================================
//  Parser  (recursive descent) -> AST
// ============================================================================
class Parser {
    vector<Token> t;
    size_t p = 0;

    const Token &cur(size_t k = 0) const { return t[min(p + k, t.size() - 1)]; }
    bool is(const string &s, size_t k = 0) const {
        return (cur(k).kind == Token::Op || cur(k).kind == Token::Id) && cur(k).text == s;
    }
    bool accept(const string &s) { if (is(s)) { p++; return true; } return false; }
    [[noreturn]] void fail(const string &msg) const {
        throw runtime_error("Line " + to_string(cur().line) + ": syntax error: " + msg +
                            " (found '" + cur().text + "')");
    }
    void expect(const string &s) { if (!accept(s)) fail("expected '" + s + "'"); }

    static bool isKeyword(const string &s) {
        static const vector<string> kw = {"int", "float", "bool", "string", "void", "if", "else",
                                          "while", "return", "break", "continue", "print",
                                          "true", "false"};
        return find(kw.begin(), kw.end(), s) != kw.end();
    }
    bool atType(size_t k = 0) const {
        return cur(k).kind == Token::Id &&
               (cur(k).text == "int" || cur(k).text == "float" || cur(k).text == "bool" ||
                cur(k).text == "string" || cur(k).text == "void");
    }
    Type parseType() {
        if (!atType()) fail("expected a type name");
        string s = cur().text; p++;
        return s == "int" ? Type::Int : s == "float" ? Type::Float : s == "bool" ? Type::Bool
             : s == "string" ? Type::String : Type::Void;
    }
    string ident() {
        if (cur().kind != Token::Id || isKeyword(cur().text)) fail("expected an identifier");
        return t[p++].text;
    }

    NodePtr parseTop() {
        int line = cur().line;
        Type ty = parseType();
        string name = ident();
        if (accept("(")) {                                       // function
            auto f = mk(K::FuncDecl, line, name);
            f->declType = ty;
            if (!is(")")) {
                do {
                    int pl = cur().line;
                    Type pt = parseType();
                    auto prm = mk(K::Param, pl, ident());
                    prm->declType = pt;
                    f->kids.push_back(move(prm));
                } while (accept(","));
            }
            expect(")");
            f->kids.push_back(parseBlock());
            return f;
        }
        auto v = mk(K::VarDecl, line, name);                     // global variable
        v->declType = ty;
        if (accept("=")) v->kids.push_back(parseExpr());
        expect(";");
        return v;
    }

    NodePtr parseBlock() {
        auto b = mk(K::Block, cur().line);
        expect("{");
        while (!is("}")) {
            if (cur().kind == Token::End) fail("missing '}'");
            b->kids.push_back(parseStmt());
        }
        expect("}");
        return b;
    }

    NodePtr parseStmt() {
        int line = cur().line;
        if (is("{")) return parseBlock();
        if (accept("if")) {
            auto n = mk(K::If, line);
            expect("("); n->kids.push_back(parseExpr()); expect(")");
            n->kids.push_back(parseStmt());
            if (accept("else")) n->kids.push_back(parseStmt());
            return n;
        }
        if (accept("while")) {
            auto n = mk(K::While, line);
            expect("("); n->kids.push_back(parseExpr()); expect(")");
            n->kids.push_back(parseStmt());
            return n;
        }
        if (accept("return")) {
            auto n = mk(K::Return, line);
            if (!is(";")) n->kids.push_back(parseExpr());
            expect(";");
            return n;
        }
        if (accept("break"))    { expect(";"); return mk(K::Break, line); }
        if (accept("continue")) { expect(";"); return mk(K::Continue, line); }
        if (accept("print")) {
            auto n = mk(K::Print, line);
            expect("("); n->kids.push_back(parseExpr()); expect(")"); expect(";");
            return n;
        }
        if (atType()) {                                          // local declaration
            Type ty = parseType();
            auto v = mk(K::VarDecl, line, ident());
            v->declType = ty;
            if (accept("=")) v->kids.push_back(parseExpr());
            expect(";");
            return v;
        }
        if (cur().kind == Token::Id && !isKeyword(cur().text) && is("=", 1)) {   // assignment
            auto a = mk(K::Assign, line, cur().text);
            p += 2;
            a->kids.push_back(parseExpr());
            expect(";");
            return a;
        }
        auto e = mk(K::ExprStmt, line);
        e->kids.push_back(parseExpr());
        expect(";");
        return e;
    }

    NodePtr parseExpr() { return parseBin(0); }

    NodePtr parseBin(int lvl) {
        static const vector<vector<string>> levels = {
            {"||"}, {"&&"}, {"==", "!="}, {"<", ">", "<=", ">="}, {"+", "-"}, {"*", "/", "%"}};
        if (lvl == (int)levels.size()) return parseUnary();
        NodePtr left = parseBin(lvl + 1);
        while (cur().kind == Token::Op &&
               find(levels[lvl].begin(), levels[lvl].end(), cur().text) != levels[lvl].end()) {
            string op = cur().text; int line = cur().line; p++;
            NodePtr right = parseBin(lvl + 1);
            auto b = mk(K::Binary, line, op);
            b->kids.push_back(move(left));
            b->kids.push_back(move(right));
            left = move(b);
        }
        return left;
    }

    NodePtr parseUnary() {
        if (is("-") || is("!")) {
            auto u = mk(K::Unary, cur().line, cur().text);
            p++;
            u->kids.push_back(parseUnary());
            return u;
        }
        return parsePrimary();
    }

    NodePtr parsePrimary() {
        int line = cur().line;
        if (cur().kind == Token::IntL)   return mk(K::IntLit, line, t[p++].text);
        if (cur().kind == Token::FloatL) return mk(K::FloatLit, line, t[p++].text);
        if (cur().kind == Token::StrL)   return mk(K::StrLit, line, t[p++].text);
        if (accept("true"))  return mk(K::BoolLit, line, "true");
        if (accept("false")) return mk(K::BoolLit, line, "false");
        if (accept("(")) { auto e = parseExpr(); expect(")"); return e; }
        if (cur().kind == Token::Id && !isKeyword(cur().text)) {
            string name = t[p++].text;
            if (accept("(")) {
                auto c = mk(K::Call, line, name);
                if (!is(")")) { do c->kids.push_back(parseExpr()); while (accept(",")); }
                expect(")");
                return c;
            }
            return mk(K::Ident, line, name);
        }
        fail("expected an expression");
    }

public:
    explicit Parser(vector<Token> toks) : t(move(toks)) {}
    NodePtr parseProgram() {
        auto prog = mk(K::Program, 1);
        while (cur().kind != Token::End) prog->kids.push_back(parseTop());
        return prog;
    }
};

// ============================================================================
//  Semantic analyzer
// ============================================================================
struct Symbol {
    string name;
    Type type;                  // variable type, or return type for functions
    bool isFunc = false, isParam = false;
    vector<Type> params;        // function parameter types
    bool initialized = false, used = false, warnedUninit = false;
    int line = 0, level = 0;
    string scopeName;           // "global" or the enclosing function
};

class SemanticAnalyzer {
    vector<Symbol> table;                       // every symbol ever declared
    vector<map<string, int>> scopes;            // name -> index into table
    vector<pair<int, string>> errors, warnings;
    Type curRet = Type::Void;
    string curFunc;
    int loopDepth = 0;

    void error(int line, const string &m)   { errors.push_back({line, m}); }
    void warning(int line, const string &m) { warnings.push_back({line, m}); }

    void pushScope() { scopes.emplace_back(); }
    void popScope() {
        for (auto &[name, idx] : scopes.back()) {
            const Symbol &s = table[idx];
            if (!s.isFunc && !s.isParam && !s.used && s.level > 0)
                warning(s.line, "variable '" + name + "' is declared but never used");
        }
        scopes.pop_back();
    }
    int lookup(const string &name) const {
        for (int i = (int)scopes.size() - 1; i >= 0; i--) {
            auto it = scopes[i].find(name);
            if (it != scopes[i].end()) return it->second;
        }
        return -1;
    }
    int declare(Symbol s) {
        auto &top = scopes.back();
        auto it = top.find(s.name);
        if (it != top.end()) {
            error(s.line, "redeclaration of '" + s.name + "' (previously declared at line " +
                          to_string(table[it->second].line) + ")");
            return it->second;
        }
        s.level = (int)scopes.size() - 1;
        s.scopeName = curFunc.empty() ? "global" : curFunc;
        table.push_back(s);
        top[s.name] = (int)table.size() - 1;
        return (int)table.size() - 1;
    }

    // Is a value of type 'src' acceptable where 'dst' is expected?
    bool assignable(Type dst, Type src, int line, const string &ctx) {
        if (dst == Type::Error || src == Type::Error) return true;       // avoid cascades
        if (src == Type::Void) { error(line, "cannot use a void value in " + ctx); return false; }
        if (dst == src) return true;
        if (dst == Type::Float && src == Type::Int) return true;         // widening
        if (dst == Type::Int && src == Type::Float) {
            error(line, ctx + ": cannot convert 'float' to 'int' (possible loss of precision)");
            return false;
        }
        error(line, ctx + ": type mismatch, cannot convert '" + tname(src) + "' to '" + tname(dst) + "'");
        return false;
    }

    static bool alwaysReturns(const Node *n) {
        switch (n->kind) {
            case K::Return: return true;
            case K::Block:
                for (auto &k : n->kids) if (alwaysReturns(k.get())) return true;
                return false;
            case K::If: return n->kids.size() == 3 && alwaysReturns(n->kids[1].get()) &&
                               alwaysReturns(n->kids[2].get());
            default: return false;
        }
    }

    // ----- expressions: return the type and annotate the node -----
    Type expr(Node *n) { return n->type = exprImpl(n); }

    Type exprImpl(Node *n) {
        switch (n->kind) {
            case K::IntLit:   return Type::Int;
            case K::FloatLit: return Type::Float;
            case K::BoolLit:  return Type::Bool;
            case K::StrLit:   return Type::String;
            case K::Ident: {
                int idx = lookup(n->text);
                if (idx < 0) { error(n->line, "undeclared variable '" + n->text + "'"); return Type::Error; }
                Symbol &s = table[idx];
                if (s.isFunc) { error(n->line, "'" + n->text + "' is a function, not a variable"); return Type::Error; }
                s.used = true;
                if (!s.initialized && !s.warnedUninit) {
                    warning(n->line, "variable '" + n->text + "' may be used before initialization");
                    s.warnedUninit = true;
                }
                return s.type;
            }
            case K::Unary: {
                Type t = expr(n->kids[0].get());
                if (t == Type::Error) return Type::Error;
                if (n->text == "-") {
                    if (isNumeric(t)) return t;
                    error(n->line, "unary '-' cannot be applied to '" + tname(t) + "'");
                } else {
                    if (t == Type::Bool) return Type::Bool;
                    error(n->line, "'!' requires a bool operand, found '" + tname(t) + "'");
                }
                return Type::Error;
            }
            case K::Binary: return binary(n);
            case K::Call:   return call(n);
            default:        return Type::Error;
        }
    }

    static bool isZeroLiteral(const Node *n) {
        if (n->kind == K::IntLit)   return stoi(n->text) == 0;
        if (n->kind == K::FloatLit) return stod(n->text) == 0.0;
        return false;
    }

    Type binary(Node *n) {
        Type l = expr(n->kids[0].get()), r = expr(n->kids[1].get());
        if (l == Type::Error || r == Type::Error) return Type::Error;
        const string &op = n->text;
        auto bad = [&]() {
            error(n->line, "operator '" + op + "' cannot be applied to '" + tname(l) + "' and '" + tname(r) + "'");
            return Type::Error;
        };
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            if (op == "/" && isZeroLiteral(n->kids[1].get())) {
                error(n->line, "division by zero");
                return Type::Error;
            }
            if (isNumeric(l) && isNumeric(r)) return (l == Type::Float || r == Type::Float) ? Type::Float : Type::Int;
            if (op == "+" && l == Type::String && r == Type::String) return Type::String;
            return bad();
        }
        if (op == "%") {
            if (l == Type::Int && r == Type::Int) {
                if (isZeroLiteral(n->kids[1].get())) { error(n->line, "modulo by zero"); return Type::Error; }
                return Type::Int;
            }
            error(n->line, "operator '%' requires int operands, found '" + tname(l) + "' and '" + tname(r) + "'");
            return Type::Error;
        }
        if (op == "<" || op == ">" || op == "<=" || op == ">=")
            return (isNumeric(l) && isNumeric(r)) ? Type::Bool : bad();
        if (op == "==" || op == "!=") {
            if ((isNumeric(l) && isNumeric(r)) || (l == r && l != Type::Void)) return Type::Bool;
            return bad();
        }
        /* && || */
        return (l == Type::Bool && r == Type::Bool) ? Type::Bool : bad();
    }

    Type call(Node *n) {
        vector<Type> argTypes;
        for (auto &a : n->kids) argTypes.push_back(expr(a.get()));
        int idx = lookup(n->text);
        if (idx < 0) { error(n->line, "call to undeclared function '" + n->text + "'"); return Type::Error; }
        Symbol &f = table[idx];
        if (!f.isFunc) { error(n->line, "'" + n->text + "' is not a function"); return Type::Error; }
        f.used = true;
        if (argTypes.size() != f.params.size()) {
            error(n->line, "function '" + n->text + "' expects " + to_string(f.params.size()) +
                           " argument(s), but " + to_string(argTypes.size()) + " given");
        } else {
            for (size_t i = 0; i < argTypes.size(); i++)
                assignable(f.params[i], argTypes[i], n->kids[i]->line,
                           "argument " + to_string(i + 1) + " of '" + n->text + "'");
        }
        return f.type;
    }

    // ----- statements -----
    void stmt(Node *n) {
        switch (n->kind) {
            case K::Block:
                pushScope();
                for (auto &k : n->kids) stmt(k.get());
                popScope();
                break;
            case K::VarDecl: varDecl(n); break;
            case K::Assign: {
                Type rt = expr(n->kids[0].get());
                int idx = lookup(n->text);
                if (idx < 0) { error(n->line, "assignment to undeclared variable '" + n->text + "'"); break; }
                Symbol &s = table[idx];
                if (s.isFunc) { error(n->line, "cannot assign to function '" + n->text + "'"); break; }
                n->type = s.type;
                assignable(s.type, rt, n->line, "assignment to '" + n->text + "'");
                s.initialized = true;
                break;
            }
            case K::If:
            case K::While: {
                Type c = expr(n->kids[0].get());
                if (c != Type::Error && c != Type::Bool)
                    error(n->kids[0]->line, string("condition of '") + (n->kind == K::If ? "if" : "while") +
                                            "' must be bool, found '" + tname(c) + "'");
                bool loop = n->kind == K::While;
                if (loop) loopDepth++;
                for (size_t i = 1; i < n->kids.size(); i++) {      // each branch gets its own scope
                    pushScope(); stmt(n->kids[i].get()); popScope();
                }
                if (loop) loopDepth--;
                break;
            }
            case K::Return: {
                if (n->kids.empty()) {
                    if (curRet != Type::Void)
                        error(n->line, "function '" + curFunc + "' must return a value of type '" + tname(curRet) + "'");
                } else {
                    Type t = expr(n->kids[0].get());
                    if (curRet == Type::Void) {
                        if (t != Type::Error) error(n->line, "void function '" + curFunc + "' cannot return a value");
                    } else assignable(curRet, t, n->line, "return statement in '" + curFunc + "'");
                }
                break;
            }
            case K::Break:    if (loopDepth == 0) error(n->line, "'break' outside of a loop"); break;
            case K::Continue: if (loopDepth == 0) error(n->line, "'continue' outside of a loop"); break;
            case K::Print: {
                Type t = expr(n->kids[0].get());
                if (t == Type::Void) error(n->line, "cannot print a void value");
                break;
            }
            case K::ExprStmt:
                expr(n->kids[0].get());
                if (n->kids[0]->kind != K::Call) warning(n->line, "expression statement has no effect");
                break;
            default: break;
        }
    }

    void varDecl(Node *n) {
        Type init = Type::Error;
        bool hasInit = !n->kids.empty();
        if (hasInit) init = expr(n->kids[0].get());            // evaluated before the name is visible
        if (n->declType == Type::Void) error(n->line, "variable '" + n->text + "' cannot have type void");
        else if (hasInit) assignable(n->declType, init, n->line, "initialization of '" + n->text + "'");
        Symbol s;
        s.name = n->text; s.type = n->declType; s.line = n->line; s.initialized = hasInit;
        declare(s);
    }

    void function(Node *f) {
        curFunc = f->text; curRet = f->declType; loopDepth = 0;
        pushScope();
        size_t nparams = f->kids.size() - 1;
        for (size_t i = 0; i < nparams; i++) {
            Node *p = f->kids[i].get();
            Symbol s;
            s.name = p->text; s.type = p->declType; s.isParam = true; s.initialized = s.used = true;
            s.line = p->line;
            if (p->declType == Type::Void) error(p->line, "parameter '" + p->text + "' cannot have type void");
            declare(s);
        }
        Node *body = f->kids.back().get();
        pushScope();                                           // body scope (params stay in outer scope)
        for (auto &k : body->kids) stmt(k.get());
        popScope();
        if (curRet != Type::Void && !alwaysReturns(body))
            error(f->line, "function '" + f->text + "' may reach the end without returning a value");
        popScope();
        curFunc.clear(); curRet = Type::Void;
    }

public:
    void analyze(Node *prog) {
        pushScope();                                           // global scope
        // pre-pass: record function signatures so they can be called before their definition
        for (auto &d : prog->kids) {
            if (d->kind != K::FuncDecl) continue;
            Symbol s;
            s.name = d->text; s.type = d->declType; s.isFunc = true; s.line = d->line;
            s.initialized = true;
            for (size_t i = 0; i + 1 < d->kids.size(); i++) s.params.push_back(d->kids[i]->declType);
            declare(s);
        }
        for (auto &d : prog->kids) {
            if (d->kind == K::VarDecl) varDecl(d.get());
            else function(d.get());
        }
        int m = lookup("main");
        if (m < 0 || !table[m].isFunc) warning(1, "program has no 'main' function");
        else table[m].used = true;
        popScope();
    }

    const vector<Symbol> &symbols() const { return table; }
    vector<pair<int, string>> getErrors() const   { auto v = errors;   stable_sort(v.begin(), v.end(), [](auto &a, auto &b) { return a.first < b.first; }); return v; }
    vector<pair<int, string>> getWarnings() const { auto v = warnings; stable_sort(v.begin(), v.end(), [](auto &a, auto &b) { return a.first < b.first; }); return v; }
};

// ============================================================================
//  Output helpers
// ============================================================================
static string label(const Node *n) {
    switch (n->kind) {
        case K::Program:  return "Program";
        case K::FuncDecl: return "Function " + n->text + " -> " + tname(n->declType);
        case K::Param:    return "Param " + n->text + " : " + tname(n->declType);
        case K::VarDecl:  return "VarDecl " + n->text + " : " + tname(n->declType);
        case K::Block:    return "Block";
        case K::If:       return "If";
        case K::While:    return "While";
        case K::Return:   return "Return";
        case K::Break:    return "Break";
        case K::Continue: return "Continue";
        case K::Print:    return "Print";
        case K::ExprStmt: return "ExprStmt";
        case K::Assign:   return "Assign " + n->text + " : " + tname(n->type);
        case K::Binary:   return "Binary '" + n->text + "' : " + tname(n->type);
        case K::Unary:    return "Unary '" + n->text + "' : " + tname(n->type);
        case K::Call:     return "Call " + n->text + " : " + tname(n->type);
        case K::Ident:    return "Ident " + n->text + " : " + tname(n->type);
        case K::IntLit:   return "IntLit " + n->text + " : int";
        case K::FloatLit: return "FloatLit " + n->text + " : float";
        case K::BoolLit:  return "BoolLit " + n->text + " : bool";
        case K::StrLit:   return "StrLit \"" + n->text + "\" : string";
    }
    return "?";
}

static void printAST(const Node *n, const string &prefix, bool last, bool root) {
    cout << prefix << (root ? "" : (last ? "`-- " : "|-- ")) << label(n) << "   [line " << n->line << "]\n";
    string next = prefix + (root ? "" : (last ? "    " : "|   "));
    for (size_t i = 0; i < n->kids.size(); i++)
        printAST(n->kids[i].get(), next, i + 1 == n->kids.size(), false);
}

int main(int argc, char *argv[]) {
    if (argc < 2) { cerr << "Usage: " << argv[0] << " <source-file.mc>\n"; return 1; }
    ifstream f(argv[1]);
    if (!f) { cerr << "cannot open " << argv[1] << "\n"; return 1; }
    stringstream ss; ss << f.rdbuf();

    NodePtr ast;
    try {
        ast = Parser(lexSource(ss.str())).parseProgram();
    } catch (const exception &e) {
        cerr << "Parsing failed - " << e.what() << "\n";
        return 2;
    }

    SemanticAnalyzer sa;
    sa.analyze(ast.get());

    cout << "================ ANNOTATED AST ================\n";
    printAST(ast.get(), "", true, true);

    cout << "\n================ SYMBOL TABLE ================\n"
         << left << setw(12) << "NAME" << setw(10) << "KIND" << setw(10) << "TYPE" << setw(14)
         << "SCOPE" << setw(7) << "LINE" << "SIGNATURE / STATUS\n" << string(70, '-') << "\n";
    for (const Symbol &s : sa.symbols()) {
        string extra;
        if (s.isFunc) {
            extra = "(";
            for (size_t i = 0; i < s.params.size(); i++) extra += (i ? ", " : "") + tname(s.params[i]);
            extra += ")";
        } else extra = s.isParam ? "parameter" : (s.used ? "used" : "unused");
        cout << setw(12) << s.name << setw(10) << (s.isFunc ? "function" : s.isParam ? "param" : "variable")
             << setw(10) << tname(s.type) << setw(14) << s.scopeName << setw(7) << s.line << extra << "\n";
    }

    auto errs = sa.getErrors();
    auto warns = sa.getWarnings();
    cout << "\n================ SEMANTIC REPORT ================\n";
    for (auto &[line, m] : errs)  cout << "Line " << line << ": error:   " << m << "\n";
    for (auto &[line, m] : warns) cout << "Line " << line << ": warning: " << m << "\n";
    cout << "\n" << errs.size() << " error(s), " << warns.size() << " warning(s)  =>  "
         << (errs.empty() ? "SEMANTIC ANALYSIS PASSED" : "SEMANTIC ANALYSIS FAILED") << "\n";
    return errs.empty() ? 0 : 3;
}