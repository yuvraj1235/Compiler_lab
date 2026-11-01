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
 * IDEA (recursive descent, precedence is built into the grammar)
 *      expr   -> term   { (+|-) term }
 *      term   -> factor { (*|/) factor }
 *      factor -> id | num | ( expr ) | - factor
 *   Each function returns the NAME that holds its value (a variable, a
 *   number or a temporary tN) and every operator adds one quadruple.
 *
 * Build : g++ -o tac 12_three_address_code.cpp
 * Run   : ./tac "a = b + c * d"
 *         ./tac                  (type statements, 'exit' to stop)
 */
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cctype>
using namespace std;

struct Quad { string op, arg1, arg2, result; };
vector<Quad> quads;
int ntemp = 0;

// ---------- lexer: look = 'i' id, 'n' number, '$' end, else the character ----------
string input;
int pos = 0;
char look;
string word;

void advance() {
    while (pos < (int)input.size() && isspace(input[pos])) pos++;
    if (pos >= (int)input.size()) { look = '$'; return; }
    char c = input[pos];
    word = "";
    if (isalpha(c) || c == '_') {
        while (pos < (int)input.size() && (isalnum(input[pos]) || input[pos] == '_')) word += input[pos++];
        look = 'i';
    } else if (isdigit(c)) {
        while (pos < (int)input.size() && (isdigit(input[pos]) || input[pos] == '.')) word += input[pos++];
        look = 'n';
    } else { look = c; pos++; }
}

string newTemp() { return "t" + to_string(++ntemp); }

void emit(string op, string a1, string a2, string res) {
    Quad q;
    q.op = op; q.arg1 = a1; q.arg2 = a2; q.result = res;
    quads.push_back(q);
}

// ---------- parser: generates code while parsing ----------
string expr();

string factor() {
    if (look == 'i' || look == 'n') { string s = word; advance(); return s; }
    if (look == '(') {
        advance();
        string e = expr();
        if (look != ')') throw string("expected ')'");
        advance();
        return e;
    }
    if (look == '-') {                              // unary minus
        advance();
        string v = factor(), t = newTemp();
        emit("uminus", v, "", t);
        return t;
    }
    throw string("expected identifier, number or '('");
}

string term() {
    string left = factor();
    while (look == '*' || look == '/') {
        string op(1, look);
        advance();
        string right = factor(), t = newTemp();
        emit(op, left, right, t);
        left = t;
    }
    return left;
}

string expr() {
    string left = term();
    while (look == '+' || look == '-') {
        string op(1, look);
        advance();
        string right = term(), t = newTemp();
        emit(op, left, right, t);
        left = t;
    }
    return left;
}

// ---------- output ----------
string ref(string name) {                           // temporaries -> (index) for triples
    for (int k = 0; k < (int)quads.size(); k++)
        if (quads[k].op != "=" && quads[k].result == name) return "(" + to_string(k) + ")";
    return name;
}

void printAll() {
    cout << "\nThree-Address Code:\n";
    for (int i = 0; i < (int)quads.size(); i++) {
        Quad &q = quads[i];
        cout << "  " << i + 1 << ": " << q.result << " = ";
        if (q.op == "=") cout << q.arg1;
        else if (q.op == "uminus") cout << "- " << q.arg1;
        else cout << q.arg1 << " " << q.op << " " << q.arg2;
        cout << "\n";
    }

    cout << "\nQuadruples:\n  " << left << setw(5) << "No." << setw(8) << "op" << setw(8) << "arg1"
         << setw(8) << "arg2" << "result\n";
    for (int i = 0; i < (int)quads.size(); i++)
        cout << "  " << setw(5) << i << setw(8) << quads[i].op << setw(8) << quads[i].arg1
             << setw(8) << (quads[i].arg2 == "" ? "-" : quads[i].arg2) << quads[i].result << "\n";

    cout << "\nTriples:\n  " << left << setw(5) << "No." << setw(8) << "op" << setw(8) << "arg1" << "arg2\n";
    for (int i = 0; i < (int)quads.size(); i++) {
        Quad &q = quads[i];
        string a1, a2;
        if (q.op == "=") { a1 = q.result; a2 = ref(q.arg1); }       // (=, target, value)
        else { a1 = ref(q.arg1); a2 = (q.arg2 == "") ? "-" : ref(q.arg2); }
        cout << "  " << setw(5) << ("(" + to_string(i) + ")") << setw(8) << q.op << setw(8) << a1 << a2 << "\n";
    }
}

void process(string text) {
    quads.clear();
    ntemp = 0;
    string target = "";
    int eq = text.find('=');
    if (eq != (int)string::npos) {                  // assignment:  target = expression
        for (int i = 0; i < eq; i++) if (!isspace(text[i])) target += text[i];
        text = text.substr(eq + 1);
        bool ok = !target.empty() && (isalpha(target[0]) || target[0] == '_');
        for (int i = 0; i < (int)target.size(); i++) if (!isalnum(target[i]) && target[i] != '_') ok = false;
        if (!ok) { cout << "Syntax error: invalid assignment target\n"; return; }
    }
    input = text;
    pos = 0;
    try {
        advance();
        string result = expr();
        if (look != '$') throw string("unexpected token after expression");
        if (target != "") emit("=", result, "", target);
        printAll();
    } catch (string msg) {
        cout << "Syntax error: " << msg << "\n";
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        string s = "";
        for (int i = 1; i < argc; i++) s += string(argv[i]) + " ";
        process(s);
        return 0;
    }
    string line;
    cout << "Enter an expression or assignment (e.g. a = b + c * d), 'exit' to stop.\n> ";
    while (getline(cin, line) && line != "exit") {
        if (line != "") process(line);
        cout << "\n> ";
    }
}
