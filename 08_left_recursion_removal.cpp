/*
 * QUESTION:
 * 2) Write a program to transform the following non-LL(1)/left-recursive
 *    grammar into an equivalent LL(1) form.
 *        E -> E + T | T
 *        T -> T * F | F
 *        F -> ( E ) | id
 *
 * METHOD
 *   For  A -> A a1 | A a2 | ... | b1 | b2 | ...   (left recursion)  rewrite as
 *        A  -> b1 A' | b2 A' | ...
 *        A' -> a1 A' | a2 A' | ... | eps
 *   Indirect left recursion is handled by first substituting earlier
 *   non-terminals (standard algorithm). The result is then verified to be
 *   LL(1) by computing FIRST/FOLLOW and checking for table conflicts.
 *
 * Build : g++ -std=c++17 -Wall -o lrr 08_left_recursion_removal.cpp
 * Run   : ./lrr
 */
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
using namespace std;

using Prod = vector<string>;
static const string EPS = "eps", END_MARK = "$";
static const char *GRAMMAR =
    "E -> E + T | T\n"
    "T -> T * F | F\n"
    "F -> ( E ) | id\n";

map<string, vector<Prod>> g;
vector<string> order;

string str(const Prod &p) { string s; for (auto &x : p) s += (s.empty() ? "" : " ") + x; return s; }

void print(const string &title) {
    cout << title << "\n";
    for (auto &A : order) {
        cout << "  " << left << setw(4) << A << " -> ";
        for (size_t i = 0; i < g[A].size(); i++) cout << (i ? " | " : "") << str(g[A][i]);
        cout << "\n";
    }
}

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

void removeLeftRecursion() {
    for (size_t i = 0; i < order.size(); i++) {
        string Ai = order[i];
        for (size_t j = 0; j < i; j++) {              // substitute earlier non-terminals
            string Aj = order[j];
            vector<Prod> updated;
            for (auto &p : g[Ai]) {
                if (p[0] == Aj)
                    for (auto &d : g[Aj]) {
                        Prod np = d;
                        np.insert(np.end(), p.begin() + 1, p.end());
                        updated.push_back(np);
                    }
                else updated.push_back(p);
            }
            g[Ai] = updated;
        }
        vector<Prod> rec, non;                        // A -> A alpha   /   A -> beta
        for (auto &p : g[Ai]) {
            if (p[0] == Ai) rec.push_back(Prod(p.begin() + 1, p.end()));
            else non.push_back(p);
        }
        if (rec.empty()) continue;
        string Anew = Ai + "'";
        vector<Prod> a;
        for (auto &b : non) {
            Prod np = (b == Prod{EPS}) ? Prod{} : b;
            np.push_back(Anew);
            a.push_back(np);
        }
        g[Ai] = a;
        vector<Prod> ap;
        for (auto &alpha : rec) { Prod np = alpha; np.push_back(Anew); ap.push_back(np); }
        ap.push_back({EPS});
        g[Anew] = ap;
        order.insert(order.begin() + i + 1, Anew);    // A' directly after A
        i++;
    }
}

// ---------- LL(1) verification ----------
map<string, set<string>> firstS, followS;
bool isNT(const string &s) { return g.count(s) > 0; }

set<string> firstSeq(const Prod &seq, size_t from = 0) {
    set<string> out;
    for (size_t i = from; i < seq.size(); i++) {
        set<string> f = isNT(seq[i]) ? firstS[seq[i]] : set<string>{seq[i]};
        for (auto &x : f) if (x != EPS) out.insert(x);
        if (!f.count(EPS)) return out;
    }
    out.insert(EPS);
    return out;
}

bool checkLL1() {
    bool ch = true;
    while (ch) {
        ch = false;
        for (auto &[A, ps] : g)
            for (auto &p : ps) {
                set<string> f = (p == Prod{EPS}) ? set<string>{EPS} : firstSeq(p);
                for (auto &x : f) ch |= firstS[A].insert(x).second;
            }
    }
    followS[order[0]].insert(END_MARK);
    ch = true;
    while (ch) {
        ch = false;
        for (auto &[A, ps] : g)
            for (auto &p : ps)
                for (size_t i = 0; i < p.size(); i++) {
                    if (!isNT(p[i])) continue;
                    set<string> r = firstSeq(p, i + 1);
                    for (auto &x : r) if (x != EPS) ch |= followS[p[i]].insert(x).second;
                    if (r.count(EPS)) for (auto &x : followS[A]) ch |= followS[p[i]].insert(x).second;
                }
    }
    bool ok = true;
    map<pair<string, string>, Prod> seen;
    for (auto &[A, ps] : g)
        for (auto &p : ps) {
            set<string> f = (p == Prod{EPS}) ? set<string>{EPS} : firstSeq(p);
            set<string> sel;
            for (auto &x : f) if (x != EPS) sel.insert(x);
            if (f.count(EPS)) sel.insert(followS[A].begin(), followS[A].end());
            for (auto &t : sel) {
                auto k = make_pair(A, t);
                if (seen.count(k) && seen[k] != p) ok = false;
                seen[k] = p;
            }
        }
    return ok;
}

void show(const string &name, map<string, set<string>> &m) {
    cout << name << ":\n";
    for (auto &A : order) {
        cout << "  " << name << "(" << A << ") = { ";
        bool f = true;
        for (auto &x : m[A]) { cout << (f ? "" : ", ") << x; f = false; }
        cout << " }\n";
    }
}

int main() {
    readGrammar();
    print("Original (left-recursive) grammar:");
    removeLeftRecursion();
    cout << "\n";
    print("Equivalent grammar without left recursion:");
    bool ok = checkLL1();
    cout << "\n";
    show("FIRST", firstS);
    show("FOLLOW", followS);
    cout << "\nLL(1) check: " << (ok ? "PASSED - the grammar is LL(1)" : "FAILED - conflicts remain") << "\n";
}
