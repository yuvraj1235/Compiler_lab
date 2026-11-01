/*
 * QUESTION:
 * 1) Consider the following grammar and construct LR(0) parsing table.
 *    Also, parse the input string abb using the parse table.
 *        S -> A
 *        A -> A b | a
 *
 * STEPS
 *   1. Augment the grammar            S' -> S
 *   2. LR(0) items, CLOSURE and GOTO -> canonical collection of item sets
 *   3. Fill ACTION / GOTO (pure LR(0): reduce on EVERY terminal)
 *   4. Report conflicts. State I2 = { S -> A. , A -> A.b } has a shift/reduce
 *      conflict in pure LR(0); it is resolved with FOLLOW(S) = {$}
 *      (SLR(1)-style), which is the table normally written in textbooks.
 *   5. Parse the input with a shift/reduce trace.
 *
 * Build : g++ -std=c++17 -Wall -o lr0 09_lr0_parser.cpp
 * Run   : ./lr0            (parses abb)
 *         ./lr0 abbb       (another string)
 */
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
using namespace std;

using Item = pair<int, int>;               // (production index, dot position)
using ItemSet = set<Item>;

struct Production { string lhs; vector<string> rhs; };

vector<Production> prods = {
    {"S'", {"S"}},          // 0 : augmented production
    {"S",  {"A"}},          // 1
    {"A",  {"A", "b"}},     // 2
    {"A",  {"a"}},          // 3
};
const string END_MARK = "$";
set<string> nonterms = {"S'", "S", "A"};
vector<string> ntOrder = {"S", "A"};       // order used for numbering and printing
vector<string> terms   = {"a", "b"};

vector<ItemSet> states;
map<pair<int, string>, int> trans;

string showProd(int i) {
    string s = prods[i].lhs + " ->";
    for (auto &x : prods[i].rhs) s += " " + x;
    return s;
}
string showItem(const Item &it) {
    const Production &p = prods[it.first];
    string s = p.lhs + " ->";
    for (int i = 0; i <= (int)p.rhs.size(); i++) {
        if (i == it.second) s += " .";
        if (i < (int)p.rhs.size()) s += " " + p.rhs[i];
    }
    return s;
}

ItemSet closure(ItemSet items) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto [p, d] : vector<Item>(items.begin(), items.end())) {
            const auto &r = prods[p].rhs;
            if (d < (int)r.size() && nonterms.count(r[d]))
                for (int k = 0; k < (int)prods.size(); k++)
                    if (prods[k].lhs == r[d]) changed |= items.insert({k, 0}).second;
        }
    }
    return items;
}

ItemSet gotoSet(const ItemSet &I, const string &X) {
    ItemSet moved;
    for (auto [p, d] : I)
        if (d < (int)prods[p].rhs.size() && prods[p].rhs[d] == X) moved.insert({p, d + 1});
    return moved.empty() ? moved : closure(moved);
}

map<string, set<string>> followSets() {
    map<string, set<string>> fol;
    fol["S'"].insert(END_MARK);
    bool ch = true;
    while (ch) {
        ch = false;
        for (auto &p : prods)
            for (size_t k = 0; k < p.rhs.size(); k++) {
                const string &B = p.rhs[k];
                if (!nonterms.count(B)) continue;
                if (k + 1 < p.rhs.size()) {
                    if (!nonterms.count(p.rhs[k + 1])) ch |= fol[B].insert(p.rhs[k + 1]).second;
                } else
                    for (auto &x : fol[p.lhs]) ch |= fol[B].insert(x).second;
            }
    }
    return fol;
}

struct Tables {
    map<pair<int, string>, string> action;
    map<pair<int, string>, int> go;
    vector<string> conflicts;
};

Tables build(bool useFollow) {
    Tables t;
    auto fol = followSets();
    auto put = [&](int s, const string &sym, const string &a) {
        auto key = make_pair(s, sym);
        auto it = t.action.find(key);
        if (it != t.action.end() && it->second != a) {
            t.conflicts.push_back("state " + to_string(s) + " on '" + sym + "': " + it->second + " vs " + a);
            it->second += "/" + a;
        } else t.action[key] = a;
    };
    for (int n = 0; n < (int)states.size(); n++) {
        for (auto [p, d] : states[n]) {
            const auto &r = prods[p].rhs;
            if (d < (int)r.size()) {
                if (!nonterms.count(r[d])) put(n, r[d], "s" + to_string(trans[{n, r[d]}]));
            } else if (p == 0) {
                put(n, END_MARK, "acc");
            } else {
                set<string> look;
                if (useFollow) look = fol[prods[p].lhs];
                else { look.insert(terms.begin(), terms.end()); look.insert(END_MARK); }
                for (auto &x : look) put(n, x, "r" + to_string(p));
            }
        }
        for (auto &A : ntOrder)
            if (trans.count({n, A})) t.go[{n, A}] = trans[{n, A}];
    }
    return t;
}

void printTable(const Tables &t, const string &title) {
    cout << "\n" << title << "\n" << left << setw(7) << "STATE";
    vector<string> cols = terms; cols.push_back(END_MARK);
    for (auto &c : cols) cout << setw(9) << c;
    cout << " | ";
    for (auto &A : ntOrder) cout << setw(6) << A;
    cout << "\n";
    for (int n = 0; n < (int)states.size(); n++) {
        cout << setw(7) << n;
        for (auto &c : cols) {
            auto it = t.action.find({n, c});
            cout << setw(9) << (it == t.action.end() ? "" : it->second);
        }
        cout << " | ";
        for (auto &A : ntOrder) {
            auto it = t.go.find({n, A});
            cout << setw(6) << (it == t.go.end() ? "" : to_string(it->second));
        }
        cout << "\n";
    }
}

bool parse(const string &w, const Tables &t) {
    vector<string> toks;
    for (char c : w) toks.push_back(string(1, c));
    toks.push_back(END_MARK);
    vector<string> stack = {"0"};
    size_t i = 0;
    cout << "\nParsing '" << w << "'\n" << left << setw(24) << "STACK" << setw(12) << "INPUT" << "ACTION\n";
    while (true) {
        int s = stoi(stack.back());
        const string &a = toks[i];
        string st, inp;
        for (auto &x : stack) st += (st.empty() ? "" : " ") + x;
        for (size_t k = i; k < toks.size(); k++) inp += toks[k];
        cout << setw(24) << st << setw(12) << inp;
        auto it = t.action.find({s, a});
        if (it == t.action.end()) {
            cout << "ERROR (no action for state " << s << ", symbol '" << a << "')\n";
            return false;
        }
        const string &act = it->second;
        if (act == "acc") { cout << "ACCEPT\n"; return true; }
        if (act[0] == 's') {
            cout << "shift " << act.substr(1) << "\n";
            stack.push_back(a); stack.push_back(act.substr(1)); i++;
        } else {
            int k = stoi(act.substr(1));
            cout << "reduce by " << showProd(k) << "\n";
            for (size_t x = 0; x < 2 * prods[k].rhs.size(); x++) stack.pop_back();
            int top = stoi(stack.back());
            stack.push_back(prods[k].lhs);
            stack.push_back(to_string(t.go.at({top, prods[k].lhs})));
        }
    }
}

int main(int argc, char *argv[]) {
    states.push_back(closure({{0, 0}}));
    for (size_t i = 0; i < states.size(); i++) {
        vector<string> symbols = ntOrder;                 // non-terminals first -> textbook numbering
        symbols.insert(symbols.end(), terms.begin(), terms.end());
        for (auto &X : symbols) {
            ItemSet J = gotoSet(states[i], X);
            if (J.empty()) continue;
            auto pos = find(states.begin(), states.end(), J);
            if (pos == states.end()) { states.push_back(J); pos = states.end() - 1; }
            trans[{(int)i, X}] = (int)(pos - states.begin());
        }
    }

    cout << "Augmented grammar:\n";
    for (int k = 0; k < (int)prods.size(); k++) cout << "  (" << k << ") " << showProd(k) << "\n";
    cout << "\nCanonical collection of LR(0) item sets:\n";
    for (int n = 0; n < (int)states.size(); n++) {
        cout << "  I" << n << ":\n";
        for (auto &it : states[n]) cout << "      " << showItem(it) << "\n";
    }
    for (auto &[k, v] : trans) cout << "  GOTO(I" << k.first << ", " << k.second << ") = I" << v << "\n";

    Tables t = build(false);
    printTable(t, "LR(0) parsing table (reduce on every terminal):");
    if (!t.conflicts.empty()) {
        cout << "\nCONFLICTS in pure LR(0):\n";
        for (auto &c : t.conflicts) cout << "  " << c << "\n";
        cout << "-> resolving reduces with FOLLOW sets (SLR(1) style) ...\n";
        t = build(true);
        printTable(t, "Final parsing table (reduce only on FOLLOW):");
        if (!t.conflicts.empty()) { cout << "Still conflicting - not SLR(1).\n"; return 1; }
    } else cout << "\nNo conflicts: the grammar is LR(0).\n";

    bool ok = parse(argc > 1 ? argv[1] : "abb", t);
    cout << "RESULT: " << (ok ? "string ACCEPTED" : "string REJECTED") << "\n";
}
