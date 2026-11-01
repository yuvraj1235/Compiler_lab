/*
 * QUESTION:
 * Write a program that accepts an arithmetic expression or a simple
 * assignment statement (e.g., a = b + c * d).
 *  (a) Performs lexical analysis and parsing (you may use a recursive
 *      descent parser or operator-precedence parsing).
 *  (b) Generates Three-Address Code for the given expression.
 *  (c) Optionally converts the Three-Address Code into Quadruple and
 *      Triple representations.
 *  (d) Handles basic operators: +, -, *, /, and assignment =.
 *  (e) Use temporary variables (e.g., t1, t2, ...) for intermediate results.
 *
 * APPROACH (recursive descent; precedence is built into the grammar)
 *      stmt   -> id '=' expr | expr
 *      expr   -> term  { (+|-) term }
 *      term   -> factor { (*|/) factor }
 *      factor -> id | num | '(' expr ')' | '-' factor
 *   Every parse function returns the NAME of the place holding its value
 *   (variable, constant or temporary) and each operator emits one quadruple.
 *
 * Build : g++ -std=c++17 -Wall -o tac 12_three_address_code.cpp
 * Run   : ./tac "a = b + c * d"     (expression on the command line)
 *         ./tac                     (interactive; type 'exit' to quit)
 * Note  : avoid user variables named t1, t2 ... (they look like temporaries).
 */
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
using namespace std;

struct Quad { string op, arg1, arg2, res; };

class TACGenerator {
    enum Tok { ID, NUM, PLUS, MINUS, STAR, SLASH, ASSIGN, LP, RP, END, BAD };
    string src;
    size_t pos = 0;
    Tok look = END;
    string lexeme;
    int ntemp = 0;

    void next() {
        while (pos < src.size() && isspace((unsigned char)src[pos])) pos++;
        if (pos >= src.size()) { look = END; return; }
        char c = src[pos];
        if (isalpha((unsigned char)c) || c == '_') {
            size_t s = pos;
            while (pos < src.size() && (isalnum((unsigned char)src[pos]) || src[pos] == '_')) pos++;
            lexeme = src.substr(s, pos - s); look = ID; return;
        }
        if (isdigit((unsigned char)c)) {
            size_t s = pos;
            while (pos < src.size() && (isdigit((unsigned char)src[pos]) || src[pos] == '.')) pos++;
            lexeme = src.substr(s, pos - s); look = NUM; return;
        }
        pos++;
        switch (c) {
            case '+': look = PLUS;   break;  case '-': look = MINUS; break;
            case '*': look = STAR;   break;  case '/': look = SLASH; break;
            case '=': look = ASSIGN; break;  case '(': look = LP;    break;
            case ')': look = RP;     break;  default:  look = BAD;
        }
    }

    string newTemp() { return "t" + to_string(++ntemp); }
    void emit(const string &op, const string &a1, const string &a2, const string &res) {
        quads.push_back({op, a1, a2, res});
    }
    [[noreturn]] void fail(const string &msg) { throw runtime_error("Syntax error: " + msg); }

    string factor() {
        if (look == ID || look == NUM) { string s = lexeme; next(); return s; }
        if (look == LP) {
            next();
            string e = expr();
            if (look != RP) fail("expected ')'");
            next();
            return e;
        }
        if (look == MINUS) {                          // unary minus
            next();
            string v = factor(), t = newTemp();
            emit("uminus", v, "", t);
            return t;
        }
        fail("expected identifier, number or '('");
    }
    string term() {
        string left = factor();
        while (look == STAR || look == SLASH) {
            string op = look == STAR ? "*" : "/";
            next();
            string right = factor(), t = newTemp();
            emit(op, left, right, t);
            left = t;
        }
        return left;
    }
    string expr() {
        string left = term();
        while (look == PLUS || look == MINUS) {
            string op = look == PLUS ? "+" : "-";
            next();
            string right = term(), t = newTemp();
            emit(op, left, right, t);
            left = t;
        }
        return left;
    }

public:
    vector<Quad> quads;

    void generate(const string &text) {
        quads.clear(); ntemp = 0; src = text; pos = 0;
        next();
        // look ahead for "id =" to detect an assignment
        size_t savePos = pos; Tok saveLook = look; string saveLex = lexeme;
        string target;
        bool isAssign = false;
        if (look == ID) {
            target = lexeme;
            next();
            if (look == ASSIGN) { isAssign = true; next(); }
            else { pos = savePos; look = saveLook; lexeme = saveLex; }   // restart
        }
        string result = expr();
        if (look != END) fail("unexpected token after expression");
        if (isAssign) emit("=", result, "", target);
    }

    void printTAC() const {
        cout << "\nThree-Address Code:\n";
        for (size_t i = 0; i < quads.size(); i++) {
            const Quad &q = quads[i];
            cout << "  " << i + 1 << ": " << q.res << " = ";
            if (q.op == "=") cout << q.arg1;
            else if (q.op == "uminus") cout << "- " << q.arg1;
            else cout << q.arg1 << " " << q.op << " " << q.arg2;
            cout << "\n";
        }
    }

    void printQuads() const {
        cout << "\nQuadruples:\n  " << left << setw(6) << "No." << setw(9) << "op" << setw(9)
             << "arg1" << setw(9) << "arg2" << "result\n";
        for (size_t i = 0; i < quads.size(); i++)
            cout << "  " << setw(6) << i << setw(9) << quads[i].op << setw(9) << quads[i].arg1
                 << setw(9) << (quads[i].arg2.empty() ? "-" : quads[i].arg2) << quads[i].res << "\n";
    }

    // Triples have no result field: a temporary is replaced by the index (k)
    // of the triple that computed it.
    string ref(const string &name) const {
        for (size_t k = 0; k < quads.size(); k++)
            if (quads[k].op != "=" && quads[k].res == name) return "(" + to_string(k) + ")";
        return name;
    }

    void printTriples() const {
        cout << "\nTriples:\n  " << left << setw(6) << "No." << setw(9) << "op" << setw(9)
             << "arg1" << "arg2\n";
        for (size_t i = 0; i < quads.size(); i++) {
            const Quad &q = quads[i];
            string a1, a2;
            if (q.op == "=") { a1 = q.res; a2 = ref(q.arg1); }          // (=, target, value)
            else { a1 = ref(q.arg1); a2 = q.arg2.empty() ? "-" : ref(q.arg2); }
            cout << "  " << setw(6) << ("(" + to_string(i) + ")") << setw(9) << q.op
                 << setw(9) << a1 << a2 << "\n";
        }
    }
};

int main(int argc, char *argv[]) {
    TACGenerator gen;
    auto process = [&](const string &text) {
        try {
            gen.generate(text);
            cout << "\nInput : " << text << "\n";
            gen.printTAC();
            gen.printQuads();
            gen.printTriples();
        } catch (const exception &e) {
            cerr << e.what() << "\n";
        }
    };
    if (argc > 1) {
        string line;
        for (int i = 1; i < argc; i++) line += string(argv[i]) + " ";
        process(line);
        return 0;
    }
    cout << "Enter an expression or assignment (e.g. a = b + c * d), 'exit' to quit.\n> ";
    string line;
    while (getline(cin, line)) {
        if (line == "exit" || line == "quit") break;
        if (!line.empty()) process(line);
        cout << "\n> ";
    }
}
