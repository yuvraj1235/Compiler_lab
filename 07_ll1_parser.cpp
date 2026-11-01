/*
 * QUESTION:
 * 1) Write a program to construct an LL(1) parsing table for a given grammar
 *    and implement a table-driven predictive parser.
 *        S -> a A B e
 *        A -> b A c | d
 *        B -> f | epsilon
 *
 * STEPS: FIRST sets -> FOLLOW sets -> LL(1) table M[NonTerminal][Terminal]
 *        (conflicts reported) -> stack-based predictive parser with a trace.
 *
 * Build : g++ -std=c++17 -Wall -o ll1 07_ll1_parser.cpp
 * Run   : ./ll1                (tests several strings)
 *         ./ll1 abdcfe         (your own string; symbols are single characters)
 * The grammar can be edited in GRAMMAR below (symbols separated by spaces,
 * 'eps' = epsilon).
 */
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
using namespace std;

using Prod = vector<string>;
static const string EPS = "eps", END_MARK = "$";

static const char *GRAMMAR =
    "S -> a A B e\n"
    "A -> b A c | d\n"
    "B -> f | eps\n";

map<string, vector<Prod>> g;            // grammar
vector<string> order;                    // non-terminals in order of appearance
map<string, set<string>> firstSet, followSet;
map<pair<string, string>, Prod> table;

bool isNT(const string &s) { return g.count(s) > 0; }

void readGrammar() {
    istringstream in(GRAMMAR);
    string line;
    while (getline(in, line)) {
        size_t arrow = line.find("->");
        if (arrow == string::npos) continue;
        istringstream l(line.substr(0, arrow));
        string lhs; l >> lhs;
        order.push_back(lhs);
        istringstream r(line.substr(arrow + 2));
        Prod cur; string tok;
        while (r >> tok) {
            if (tok == "|") { g[lhs].push_back(cur); cur.clear(); }
            else cur.push_back(tok);
        }
        g[lhs].push_back(cur);
    }
}

set<string> firstOfSeq(const Prod &seq, size_t from = 0) {
    set<string> out;
    for (size_t i = from; i < seq.size(); i++) {
        set<string> f = isNT(seq[i]) ? firstSet[seq[i]] : set<string>{seq[i]};
        for (auto &x : f) if (x != EPS) out.insert(x);
        if (!f.count(EPS)) return out;
    }
    out.insert(EPS);                      // every symbol was nullable
    return out;
}

void computeFirst() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto &[A, prods] : g)
            for (auto &p : prods) {
                set<string> f = (p == Prod{EPS}) ? set<string>{EPS} : firstOfSeq(p);
                for (auto &x : f) changed |= firstSet[A].insert(x).second;
            }
    }
}

void computeFollow() {
    followSet[order[0]].insert(END_MARK);
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto &[A, prods] : g)
            for (auto &p : prods)
                for (size_t i = 0; i < p.size(); i++) {
                    if (!isNT(p[i])) continue;
                    set<string> rest = firstOfSeq(p, i + 1);
                    for (auto &x : rest)
                        if (x != EPS) changed |= followSet[p[i]].insert(x).second;
                    if (rest.count(EPS))
                        for (auto &x : followSet[A]) changed |= followSet[p[i]].insert(x).second;
                }
    }
}

string str(const Prod &p) { string s; for (auto &x : p) s += (s.empty() ? "" : " ") + x; return s; }

bool buildTable() {
    bool ok = true;
    for (auto &[A, prods] : g)
        for (auto &p : prods) {
            set<string> f = (p == Prod{EPS}) ? set<string>{EPS} : firstOfSeq(p);
            set<string> targets;
            for (auto &x : f) if (x != EPS) targets.insert(x);
            if (f.count(EPS)) targets.insert(followSet[A].begin(), followSet[A].end());
            for (auto &t : targets) {
                auto key = make_pair(A, t);
                if (table.count(key) && table[key] != p) {
                    cout << "CONFLICT at M[" << A << "," << t << "]\n";
                    ok = false;
                }
                table[key] = p;
            }
        }
    return ok;
}

void printSets(const string &name, map<string, set<string>> &sets) {
    cout << "\n" << name << " sets:\n";
    for (auto &A : order) {
        cout << "  " << name << "(" << A << ") = { ";
        bool first = true;
        for (auto &x : sets[A]) { cout << (first ? "" : ", ") << x; first = false; }
        cout << " }\n";
    }
}

void printTable(const vector<string> &terms) {
    cout << "\nLL(1) Parsing Table:\n" << setw(6) << "";
    for (auto &t : terms) cout << setw(16) << t;
    cout << setw(16) << END_MARK << "\n";
    for (auto &A : order) {
        cout << left << setw(6) << A << right;
        for (size_t i = 0; i <= terms.size(); i++) {
            string t = i < terms.size() ? terms[i] : END_MARK;
            auto it = table.find({A, t});
            cout << setw(16) << (it == table.end() ? "" : A + "->" + str(it->second));
        }
        cout << "\n";
    }
    cout << left;
}

bool parse(const string &input) {
    vector<string> tokens;
    for (char c : input) tokens.push_back(string(1, c));
    tokens.push_back(END_MARK);
    vector<string> stack = {END_MARK, order[0]};
    size_t i = 0;
    cout << "\n" << left << setw(22) << "STACK" << setw(16) << "INPUT" << "ACTION\n";
    while (true) {
        string top = stack.back(), cur = tokens[i];
        string st, inp;
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) st += (st.empty() ? "" : " ") + *it;
        for (size_t k = i; k < tokens.size(); k++) inp += tokens[k];
        cout << setw(22) << st << setw(16) << inp;
        if (top == END_MARK && cur == END_MARK) { cout << "ACCEPT\n"; return true; }
        if (!isNT(top)) {
            if (top == cur) { cout << "match '" << cur << "'\n"; stack.pop_back(); i++; }
            else { cout << "ERROR: expected '" << top << "', found '" << cur << "'\n"; return false; }
        } else {
            auto it = table.find({top, cur});
            if (it == table.end()) { cout << "ERROR: no entry M[" << top << ", " << cur << "]\n"; return false; }
            cout << "output " << top << " -> " << str(it->second) << "\n";
            stack.pop_back();
            if (it->second != Prod{EPS})
                for (auto r = it->second.rbegin(); r != it->second.rend(); ++r) stack.push_back(*r);
        }
    }
}

int main(int argc, char *argv[]) {
    readGrammar();
    computeFirst();
    computeFollow();
    printSets("FIRST", firstSet);
    printSets("FOLLOW", followSet);

    set<string> tset;
    for (auto &[A, prods] : g)
        for (auto &p : prods)
            for (auto &s : p) if (!isNT(s) && s != EPS) tset.insert(s);
    vector<string> terms(tset.begin(), tset.end());

    bool ok = buildTable();
    printTable(terms);
    if (!ok) { cout << "\nGrammar is NOT LL(1).\n"; return 1; }
    cout << "\nNo conflicts: the grammar is LL(1).\n";

    vector<string> tests;
    for (int i = 1; i < argc; i++) tests.push_back(argv[i]);
    if (tests.empty()) tests = {"abdcfe", "ade", "abbdccfe", "abdfe", "adf"};
    for (auto &s : tests) {
        cout << "\n" << string(60, '=') << "\nParsing '" << s << "'";
        bool acc = parse(s);
        cout << "RESULT: " << (acc ? "string ACCEPTED" : "string REJECTED") << "\n";
    }
}
