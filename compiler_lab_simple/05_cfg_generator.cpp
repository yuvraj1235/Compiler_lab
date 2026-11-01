/*
 * QUESTION:
 * 3) Write a program to generate a suitable CFG and the language pattern
 *    generated using the CFG in terms of n where n>0, for the following input
 *    of terminals: aabaabaa.
 *
 * IDEA
 *   aabaabaa = (aab)(aab)(aa) = (aab)^2 aa
 *   The repeating unit is everything up to and including the first 'b'.
 *   Grammar :  S -> aab A
 *              A -> aab A | aa
 *   Language:  L = { (aab)^n aa | n > 0 }      (the input is n = 2)
 *
 * Build : g++ -o cfg 05_cfg_generator.cpp
 * Run   : ./cfg            (or ./cfg aabaabaa)
 */
#include <iostream>
#include <string>
using namespace std;

int main(int argc, char *argv[]) {
    string w = argc > 1 ? argv[1] : "aabaabaa";
    cout << "Input: " << w << "\n";

    int b = w.find('b');
    if (b == (int)string::npos) { cout << "no 'b' found, no pattern\n"; return 1; }
    string unit = w.substr(0, b + 1);               // repeating unit "aab"

    int n = 0;                                      // how many times the unit repeats
    while ((n + 1) * unit.size() <= w.size() && w.compare(n * unit.size(), unit.size(), unit) == 0) n++;
    string tail = w.substr(n * unit.size());        // what is left: "aa"
    if (n == 0 || tail == "") { cout << "no repeating pattern found\n"; return 1; }

    cout << "unit = '" << unit << "', tail = '" << tail << "', repeats = " << n << "\n\n";
    cout << "CFG:\n   S -> " << unit << " A\n   A -> " << unit << " A | " << tail << "\n\n";
    cout << "Language pattern: L = { (" << unit << ")^n " << tail << " | n > 0 }   (input is n = " << n << ")\n\n";

    cout << "Derivation of " << w << ":\n   S\n";
    string cur = unit;
    for (int i = 1; i <= n; i++) {
        cout << "   => " << cur << "A\n";
        cur += unit;
    }
    cout << "   => " << cur.substr(0, cur.size() - unit.size()) + tail << "\n\n";

    cout << "Strings for n = 1..5:\n";
    for (int k = 1; k <= 5; k++) {
        string s = "";
        for (int i = 0; i < k; i++) s += unit;
        cout << "   n=" << k << "  " << s + tail << "\n";
    }
}
