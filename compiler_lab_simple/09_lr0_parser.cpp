/*
 * QUESTION:
 * 1) Consider the following grammar and construct LR(0) parsing table.
 *    Also, parse the input string abb using the parse table.
 *        S -> A
 *        A -> A b | a
 *
 * NOTATION: one character per symbol; upper case = non-terminal ('Z' is the
 *           augmented start, printed as S').
 *
 * STEPS: augment grammar -> item sets (CLOSURE / GOTO) -> LR(0) table ->
 *        parse the input with a shift/reduce trace.
 *
 * NOTE: state I2 = { S -> A. , A -> A.b } has a shift/reduce conflict in a
 *       pure LR(0) table (reduce is written under every terminal). The
 *       program shows it, then reduces only on FOLLOW(S) = {$} (SLR style),
 *       which is the table normally given as the textbook answer.
 *
 * Build : g++ -o lr0 09_lr0_parser.cpp
 * Run   : ./lr0          (parses abb)      ./lr0 abbb
 */
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>
using namespace std;

// production i :  lhs[i] -> rhs[i]     (production 0 is the augmented one)
char lhs[] = {'Z', 'S', 'A', 'A'};
string rhs[] = {"S", "A", "Ab", "a"};
int nProd = 4;

typedef set<pair<int, int> > ItemSet;       // item = (production, dot position)
vector<ItemSet> states;
map<pair<int, char>, int> trans;            // trans[(state, symbol)] = next state
map<char, set<char> > firstSet, followSet;
string symbols = "SAab";                    // grammar symbols (non-terminals first)

string prodText(int p) {
    string l = (lhs[p] == 'Z') ? "S'" : string(1, lhs[p]);
    return l + " -> " + rhs[p];
}

ItemSet closure(ItemSet items) {
    bool changed = true;
    while (changed) {
        changed = false;
        ItemSet copy = items;
        for (ItemSet::iterator it = copy.begin(); it != copy.end(); ++it) {
            int p = it->first, dot = it->second;
            if (dot < (int)rhs[p].size() && isupper(rhs[p][dot]))      // dot before a non-terminal
                for (int q = 0; q < nProd; q++)
                    if (lhs[q] == rhs[p][dot] && items.insert(make_pair(q, 0)).second) changed = true;
        }
    }
    return items;
}

ItemSet gotoSet(ItemSet &I, char X) {
    ItemSet moved;
    for (ItemSet::iterator it = I.begin(); it != I.end(); ++it)
        if (it->second < (int)rhs[it->first].size() && rhs[it->first][it->second] == X)
            moved.insert(make_pair(it->first, it->second + 1));
    if (moved.empty()) return moved;
    return closure(moved);
}

void computeFollow() {                      // grammar has no epsilon, so FIRST is simple
    bool changed = true;
    while (changed) {
        changed = false;
        for (int p = 0; p < nProd; p++) {
            char c = rhs[p][0];
            if (!isupper(c)) { if (firstSet[lhs[p]].insert(c).second) changed = true; }
            else for (set<char>::iterator it = firstSet[c].begin(); it != firstSet[c].end(); ++it)
                if (firstSet[lhs[p]].insert(*it).second) changed = true;
        }
    }
    followSet['Z'].insert('$');
    changed = true;
    while (changed) {
        changed = false;
        for (int p = 0; p < nProd; p++)
            for (int i = 0; i < (int)rhs[p].size(); i++) {
                char B = rhs[p][i];
                if (!isupper(B)) continue;
                if (i + 1 == (int)rhs[p].size()) {                       // B is last
                    set<char> f = followSet[lhs[p]];
                    for (set<char>::iterator it = f.begin(); it != f.end(); ++it)
                        if (followSet[B].insert(*it).second) changed = true;
                } else {
                    char n = rhs[p][i + 1];
                    if (!isupper(n)) { if (followSet[B].insert(n).second) changed = true; }
                    else for (set<char>::iterator it = firstSet[n].begin(); it != firstSet[n].end(); ++it)
                        if (followSet[B].insert(*it).second) changed = true;
                }
            }
    }
}

map<pair<int, char>, string> action;        // action[(state, terminal)] = s3 / r2 / acc

bool buildTable(bool useFollow) {
    action.clear();
    bool conflict = false;
    string terminals = "ab$";
    for (int s = 0; s < (int)states.size(); s++)
        for (ItemSet::iterator it = states[s].begin(); it != states[s].end(); ++it) {
            int p = it->first, dot = it->second;
            vector<pair<char, string> > put;                 // (terminal, action) to write
            if (dot < (int)rhs[p].size()) {
                char X = rhs[p][dot];
                if (!isupper(X)) put.push_back(make_pair(X, "s" + to_string(trans[make_pair(s, X)])));
            } else if (p == 0) {
                put.push_back(make_pair('$', string("acc")));
            } else {
                for (int t = 0; t < 3; t++)
                    if (!useFollow || followSet[lhs[p]].count(terminals[t]))
                        put.push_back(make_pair(terminals[t], "r" + to_string(p)));
            }
            for (int k = 0; k < (int)put.size(); k++) {
                pair<int, char> key(s, put[k].first);
                if (action.count(key) && action[key] != put[k].second) {
                    action[key] += "/" + put[k].second;
                    conflict = true;
                } else action[key] = put[k].second;
            }
        }
    return conflict;
}

void printTable() {
    cout << left << setw(7) << "STATE" << setw(8) << "a" << setw(8) << "b" << setw(8) << "$" << "| S   A\n";
    for (int s = 0; s < (int)states.size(); s++) {
        cout << setw(7) << s;
        string cols = "ab$";
        for (int c = 0; c < 3; c++) cout << setw(8) << (action.count(make_pair(s, cols[c])) ? action[make_pair(s, cols[c])] : "");
        cout << "| ";
        string nts = "SA";
        for (int c = 0; c < 2; c++)
            if (trans.count(make_pair(s, nts[c]))) cout << setw(4) << trans[make_pair(s, nts[c])];
            else cout << setw(4) << "";
        cout << "\n";
    }
}

bool parse(string w) {
    w += '$';
    vector<string> stack;
    stack.push_back("0");
    int i = 0;
    cout << "\n" << left << setw(20) << "STACK" << setw(10) << "INPUT" << "ACTION\n";
    while (true) {
        string st = "";
        for (int k = 0; k < (int)stack.size(); k++) st += stack[k] + " ";
        int s = stoi(stack[stack.size() - 1]);
        pair<int, char> key(s, w[i]);
        cout << setw(20) << st << setw(10) << w.substr(i);
        if (!action.count(key)) { cout << "ERROR\n"; return false; }
        string a = action[key];
        if (a == "acc") { cout << "ACCEPT\n"; return true; }
        if (a[0] == 's') {                                       // shift
            cout << "shift " << a.substr(1) << "\n";
            stack.push_back(string(1, w[i]));
            stack.push_back(a.substr(1));
            i++;
        } else {                                                 // reduce
            int p = stoi(a.substr(1));
            cout << "reduce " << prodText(p) << "\n";
            for (int k = 0; k < 2 * (int)rhs[p].size(); k++) stack.pop_back();
            int top = stoi(stack[stack.size() - 1]);
            stack.push_back(string(1, lhs[p]));
            stack.push_back(to_string(trans[make_pair(top, lhs[p])]));
        }
    }
}

int main(int argc, char *argv[]) {
    // build the canonical collection of LR(0) item sets
    ItemSet start;
    start.insert(make_pair(0, 0));
    states.push_back(closure(start));
    for (int i = 0; i < (int)states.size(); i++)
        for (int k = 0; k < (int)symbols.size(); k++) {
            ItemSet J = gotoSet(states[i], symbols[k]);
            if (J.empty()) continue;
            int target = -1;
            for (int s = 0; s < (int)states.size(); s++) if (states[s] == J) target = s;
            if (target == -1) { states.push_back(J); target = states.size() - 1; }
            trans[make_pair(i, symbols[k])] = target;
        }

    cout << "Augmented grammar:\n";
    for (int p = 0; p < nProd; p++) cout << "  (" << p << ") " << prodText(p) << "\n";
    cout << "\nItem sets:\n";
    for (int s = 0; s < (int)states.size(); s++) {
        cout << "  I" << s << ":\n";
        for (ItemSet::iterator it = states[s].begin(); it != states[s].end(); ++it) {
            string r = rhs[it->first];
            cout << "      " << (lhs[it->first] == 'Z' ? "S'" : string(1, lhs[it->first])) << " -> "
                 << r.substr(0, it->second) << "." << r.substr(it->second) << "\n";
        }
    }

    computeFollow();
    bool conflict = buildTable(false);
    cout << "\nLR(0) parsing table (reduce under every terminal):\n";
    printTable();
    if (conflict) {
        cout << "\nShift/reduce conflict in state 2 (pure LR(0)). Reducing only on FOLLOW(S) = {$} fixes it:\n";
        buildTable(true);
        printTable();
    }
    cout << "\nParsing";
    string w = argc > 1 ? argv[1] : "abb";
    cout << " '" << w << "'";
    cout << (parse(w) ? "RESULT: accepted\n" : "RESULT: rejected\n");
}
