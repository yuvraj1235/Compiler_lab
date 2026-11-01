/*
 * QUESTION:
 * 1) Write a program to construct an LL(1) parsing table for a given grammar
 *    and implement a table-driven predictive parser.
 *        S -> a A B e
 *        A -> b A c | d
 *        B -> f | epsilon
 *
 * NOTATION: every symbol is ONE character. Upper case = non-terminal,
 *           lower case = terminal, '#' = epsilon, '$' = end marker.
 *
 * STEPS: FIRST sets -> FOLLOW sets -> LL(1) table -> stack-based parser.
 *
 * Build : g++ -o ll1 07_ll1_parser.cpp
 * Run   : ./ll1               (several test strings)
 *         ./ll1 abdcfe        (your own string)
 */
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>
using namespace std;

// the grammar: production i is  lhs[i] -> rhs[i]
char lhs[] = {'S', 'A', 'A', 'B', 'B'};
string rhs[] = {"aABe", "bAc", "d", "f", "#"};
int nProd = 5;
char startSymbol = 'S';

map<char, set<char> > first, follow;
map<pair<char, char>, int> table;           // table[(A, a)] = production number

string show(string s) { return s == "#" ? "eps" : s; }

// FIRST of the string s starting at position 'from' ('#' in the result = can be empty)
set<char> firstOf(string s, int from) {
    set<char> r;
    if (s == "#") { r.insert('#'); return r; }
    for (int i = from; i < (int)s.size(); i++) {
        char c = s[i];
        if (!isupper(c)) { r.insert(c); return r; }             // terminal
        for (set<char>::iterator it = first[c].begin(); it != first[c].end(); ++it)
            if (*it != '#') r.insert(*it);
        if (!first[c].count('#')) return r;                     // c cannot vanish
    }
    r.insert('#');                                              // everything can vanish
    return r;
}

void computeFirst() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (int p = 0; p < nProd; p++) {
            set<char> f = firstOf(rhs[p], 0);
            for (set<char>::iterator it = f.begin(); it != f.end(); ++it)
                if (first[lhs[p]].insert(*it).second) changed = true;
        }
    }
}

void computeFollow() {
    follow[startSymbol].insert('$');
    bool changed = true;
    while (changed) {
        changed = false;
        for (int p = 0; p < nProd; p++)
            for (int i = 0; i < (int)rhs[p].size(); i++) {
                char B = rhs[p][i];
                if (!isupper(B)) continue;
                set<char> f = firstOf(rhs[p], i + 1);           // what can come after B
                for (set<char>::iterator it = f.begin(); it != f.end(); ++it)
                    if (*it != '#' && follow[B].insert(*it).second) changed = true;
                if (f.count('#'))                               // B can be last: add FOLLOW(lhs)
                    for (set<char>::iterator it = follow[lhs[p]].begin(); it != follow[lhs[p]].end(); ++it)
                        if (follow[B].insert(*it).second) changed = true;
            }
    }
}

bool buildTable() {
    bool ok = true;
    for (int p = 0; p < nProd; p++) {
        set<char> f = firstOf(rhs[p], 0);
        set<char> targets;
        for (set<char>::iterator it = f.begin(); it != f.end(); ++it)
            if (*it != '#') targets.insert(*it);
        if (f.count('#'))
            targets.insert(follow[lhs[p]].begin(), follow[lhs[p]].end());
        for (set<char>::iterator it = targets.begin(); it != targets.end(); ++it) {
            pair<char, char> key(lhs[p], *it);
            if (table.count(key)) { cout << "CONFLICT at M[" << lhs[p] << "," << *it << "]\n"; ok = false; }
            table[key] = p;
        }
    }
    return ok;
}

void printSet(string name, map<char, set<char> > &m, string nonTerminals) {
    for (int i = 0; i < (int)nonTerminals.size(); i++) {
        char A = nonTerminals[i];
        cout << "  " << name << "(" << A << ") = { ";
        for (set<char>::iterator it = m[A].begin(); it != m[A].end(); ++it)
            cout << (*it == '#' ? string("eps") : string(1, *it)) << " ";
        cout << "}\n";
    }
}

bool parse(string input) {
    input += '$';
    string stack = "$";
    stack += startSymbol;                                       // top of stack = last char
    int i = 0;
    cout << "\n" << left << setw(14) << "STACK" << setw(12) << "INPUT" << "ACTION\n";
    while (true) {
        string shown(stack.rbegin(), stack.rend());             // print top on the left
        char top = stack[stack.size() - 1], cur = input[i];
        cout << setw(14) << shown << setw(12) << input.substr(i);
        if (top == '$' && cur == '$') { cout << "ACCEPT\n"; return true; }
        if (!isupper(top)) {                                    // terminal on top
            if (top == cur) { cout << "match " << cur << "\n"; stack.erase(stack.size() - 1); i++; }
            else { cout << "ERROR: expected " << top << "\n"; return false; }
        } else {                                                // non-terminal on top
            map<pair<char, char>, int>::iterator it = table.find(make_pair(top, cur));
            if (it == table.end()) { cout << "ERROR: no entry M[" << top << "," << cur << "]\n"; return false; }
            int p = it->second;
            cout << top << " -> " << show(rhs[p]) << "\n";
            stack.erase(stack.size() - 1);
            if (rhs[p] != "#")
                for (int k = rhs[p].size() - 1; k >= 0; k--) stack += rhs[p][k];
        }
    }
}

int main(int argc, char *argv[]) {
    string nonTerminals = "SAB", terminals = "abcdef";
    computeFirst();
    computeFollow();
    cout << "FIRST sets:\n";  printSet("FIRST", first, nonTerminals);
    cout << "FOLLOW sets:\n"; printSet("FOLLOW", follow, nonTerminals);

    bool ok = buildTable();
    cout << "\nLL(1) parsing table:\n" << setw(4) << "";
    string cols = "abcdef$";
    for (int c = 0; c < (int)cols.size(); c++) cout << setw(11) << cols[c];
    cout << "\n";
    for (int r = 0; r < (int)nonTerminals.size(); r++) {
        cout << setw(4) << nonTerminals[r];
        for (int c = 0; c < (int)cols.size(); c++) {
            map<pair<char, char>, int>::iterator it = table.find(make_pair(nonTerminals[r], cols[c]));
            string cell = "";
            if (it != table.end()) cell = string(1, nonTerminals[r]) + "->" + show(rhs[it->second]);
            cout << setw(11) << cell;
        }
        cout << "\n";
    }
    if (!ok) { cout << "The grammar is NOT LL(1)\n"; return 1; }
    cout << "No conflicts: the grammar is LL(1).\n";

    vector<string> tests;
    if (argc > 1) tests.push_back(argv[1]);
    else { tests.push_back("abdcfe"); tests.push_back("ade"); tests.push_back("abbdccfe"); tests.push_back("abdfe"); }
    for (int t = 0; t < (int)tests.size(); t++) {
        cout << "\n==== parsing " << tests[t] << " ====";
        cout << (parse(tests[t]) ? "RESULT: accepted\n" : "RESULT: rejected\n");
    }
}
