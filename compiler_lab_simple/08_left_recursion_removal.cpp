/*
 * QUESTION:
 * 2) Write a program to transform the following non-LL(1)/left-recursive
 *    grammar into an equivalent LL(1) form.
 *        E -> E + T | T
 *        T -> T * F | F
 *        F -> ( E ) | id
 *
 * METHOD (immediate left recursion)
 *   A -> A a1 | A a2 | b1 | b2     becomes
 *   A  -> b1 A' | b2 A'
 *   A' -> a1 A' | a2 A' | eps
 *
 * Build : g++ -o lrr 08_left_recursion_removal.cpp
 * Run   : ./lrr
 */
#include <iostream>
#include <string>
#include <vector>
using namespace std;

typedef vector<string> Production;          // one alternative = list of symbols

struct Rule {
    string name;                            // left side, e.g. E
    vector<Production> alts;                // alternatives
};

void print(vector<Rule> &g) {
    for (int i = 0; i < (int)g.size(); i++) {
        cout << "   " << g[i].name << " -> ";
        for (int j = 0; j < (int)g[i].alts.size(); j++) {
            if (j > 0) cout << " | ";
            for (int k = 0; k < (int)g[i].alts[j].size(); k++) cout << g[i].alts[j][k] << " ";
        }
        cout << "\n";
    }
}

int main() {
    // E -> E + T | T      T -> T * F | F      F -> ( E ) | id
    vector<Rule> g(3);
    g[0].name = "E";
    g[0].alts.push_back(Production{"E", "+", "T"});
    g[0].alts.push_back(Production{"T"});
    g[1].name = "T";
    g[1].alts.push_back(Production{"T", "*", "F"});
    g[1].alts.push_back(Production{"F"});
    g[2].name = "F";
    g[2].alts.push_back(Production{"(", "E", ")"});
    g[2].alts.push_back(Production{"id"});

    cout << "Original grammar:\n";
    print(g);

    vector<Rule> result;
    for (int i = 0; i < (int)g.size(); i++) {
        string A = g[i].name;
        vector<Production> recursive, others;    // A -> A alpha   and   A -> beta
        for (int j = 0; j < (int)g[i].alts.size(); j++) {
            Production p = g[i].alts[j];
            if (p[0] == A) recursive.push_back(Production(p.begin() + 1, p.end()));
            else others.push_back(p);
        }
        if (recursive.empty()) { result.push_back(g[i]); continue; }   // no left recursion

        string Anew = A + "'";
        Rule r1, r2;
        r1.name = A;
        for (int j = 0; j < (int)others.size(); j++) {                 // A -> beta A'
            Production p = others[j];
            p.push_back(Anew);
            r1.alts.push_back(p);
        }
        r2.name = Anew;
        for (int j = 0; j < (int)recursive.size(); j++) {              // A' -> alpha A'
            Production p = recursive[j];
            p.push_back(Anew);
            r2.alts.push_back(p);
        }
        r2.alts.push_back(Production{"eps"});                          // A' -> eps
        result.push_back(r1);
        result.push_back(r2);
    }

    cout << "\nGrammar after removing left recursion (now LL(1)):\n";
    print(result);
}
