/*
 * QUESTION:
 * 3) Write a program to generate a suitable CFG and the language pattern
 *    generated using the CFG in terms of n where n>0, for the following input
 *    of terminals: aabaabaa.
 *
 * APPROACH
 *   The input  aabaabaa = (aab)(aab)(aa) = (aab)^2 aa.
 *   The repeating unit is everything up to and including the first 'b'
 *   ("aab"); the string ends with the tail "aa". The program detects the unit
 *   and the tail automatically and builds the CFG
 *         S -> aab A
 *         A -> aab A | aa
 *   Language : L = { (aab)^n aa | n > 0 }      (n = 2 is the given input)
 *
 * Build : g++ -std=c++17 -Wall -o cfg 05_cfg_generator.cpp
 * Run   : ./cfg              (uses aabaabaa)
 *         ./cfg aabaabaa
 */
#include <iostream>
#include <string>
#include <vector>
#include <set>
using namespace std;

int main(int argc, char *argv[]) {
    string w = argc > 1 ? argv[1] : "aabaabaa";
    cout << "Input terminals : " << w << "\n";

    size_t b = w.find('b');
    if (b == string::npos) { cout << "No separator 'b' found - no pattern.\n"; return 1; }
    string unit = w.substr(0, b + 1);
    int n0 = 0;
    while (w.compare(n0 * unit.size(), unit.size(), unit) == 0 &&
           (size_t)(n0 + 1) * unit.size() <= w.size()) n0++;
    string tail = w.substr(n0 * unit.size());
    if (n0 < 1 || tail.empty()) { cout << "No repeating pattern found.\n"; return 1; }

    cout << "Repeating unit  : '" << unit << "'   tail : '" << tail
         << "'   repetitions in input : " << n0 << "\n";

    set<char> T(w.begin(), w.end());
    cout << "\nContext-Free Grammar G = (V, T, P, S)\n  V = {S, A}\n  T = {";
    bool first = true;
    for (char c : T) { cout << (first ? "" : ", ") << c; first = false; }
    cout << "}\n  S = S\n  P :\n      S -> " << unit << " A\n      A -> " << unit
         << " A | " << tail << "\n";

    cout << "\nLanguage pattern : L = { (" << unit << ")^n " << tail
         << "  |  n > 0 }\n(the given input is the case n = " << n0 << ")\n";

    // leftmost derivation
    cout << "\nLeftmost derivation of '" << w << "':\n  S\n";
    string cur = unit;                       // sentential form without trailing A
    for (int i = 1; i < n0; i++) {
        cout << "    => " << cur << "A\n";
        cur += unit;
    }
    cout << "    => " << cur << "A\n";
    cur += tail;
    cout << "    => " << cur << "\n";
    if (cur != w) { cerr << "derivation mismatch!\n"; return 2; }

    cout << "\nStrings generated for n = 1 .. 5:\n";
    for (int n = 1; n <= 5; n++) {
        string s;
        for (int i = 0; i < n; i++) s += unit;
        cout << "  n=" << n << ":  " << s + tail << "\n";
    }
}
