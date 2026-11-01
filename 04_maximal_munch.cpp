/*
 * QUESTION:
 * 2) Write a program to implement a lexical analyzer component that applies
 *    the Maximal Munch (Longest Match) rule to resolve tokenization
 *    ambiguities.
 *
 * IDEA
 *   At every position every token rule is tried. Each rule reports how many
 *   characters it can match. The rule with the LONGEST match wins; on a tie
 *   the rule listed first (higher priority) wins - a keyword beats an
 *   identifier of the same length.
 *     "iffy"  -> IDENTIFIER(iffy)   not KEYWORD(if) + IDENTIFIER(fy)
 *     "<<="   -> one OPERATOR       not  <  <  =
 *     "a+++b" -> a ++ + b
 *     "12.5"  -> FLOAT(12.5)        not INT(12) . INT(5)
 *
 * Build : g++ -std=c++17 -Wall -o munch 04_maximal_munch.cpp
 * Run   : ./munch                         (built-in test string)
 *         ./munch "if (x<=y) z<<=2;"      (your own input)
 */
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <functional>
#include <cctype>
using namespace std;

using MatchFn = function<size_t(const string &, size_t)>;   // length matched, 0 = none
struct Rule { string name; MatchFn match; };

static size_t longestOf(const string &s, size_t p, const vector<string> &list) {
    size_t best = 0;
    for (const string &w : list)
        if (w.size() > best && s.compare(p, w.size(), w) == 0) best = w.size();
    return best;
}
static size_t digits(const string &s, size_t p) {
    size_t n = 0;
    while (p + n < s.size() && isdigit((unsigned char)s[p + n])) n++;
    return n;
}

int main(int argc, char *argv[]) {
    string input = argc > 1 ? argv[1]
        : "iffy if ifx==y a<<=b x>>=2 a+++b 12.5.3 p->q while1 while 7";

    const vector<string> KEYWORDS  = {"if", "else", "while", "for", "int", "return"};
    const vector<string> OPERATORS = {"<<=", ">>=", "==", "!=", "<=", ">=", "<<", ">>",
                                      "++", "--", "+=", "-=", "->", "&&", "||",
                                      "=", "<", ">", "+", "-", "*", "/", "!"};

    // ORDER = PRIORITY for ties: KEYWORD comes before IDENTIFIER.
    vector<Rule> rules = {
        {"KEYWORD",    [&](const string &s, size_t p) { return longestOf(s, p, KEYWORDS); }},
        {"IDENTIFIER", [](const string &s, size_t p) {
            if (!(isalpha((unsigned char)s[p]) || s[p] == '_')) return (size_t)0;
            size_t n = 0;
            while (p + n < s.size() && (isalnum((unsigned char)s[p + n]) || s[p + n] == '_')) n++;
            return n; }},
        {"FLOAT",      [](const string &s, size_t p) {
            size_t n = digits(s, p);
            if (n && p + n < s.size() && s[p + n] == '.') {
                size_t f = digits(s, p + n + 1);
                if (f) return n + 1 + f;
            }
            return (size_t)0; }},
        {"INTEGER",    [](const string &s, size_t p) { return digits(s, p); }},
        {"OPERATOR",   [&](const string &s, size_t p) { return longestOf(s, p, OPERATORS); }},
        {"DELIMITER",  [](const string &s, size_t p) {
            return string("(){}[];,.").find(s[p]) != string::npos ? (size_t)1 : (size_t)0; }},
    };

    cout << "Input: \"" << input << "\"\n\n"
         << left << setw(10) << "POSITION" << " | " << setw(38) << "CANDIDATE RULES (match length)"
         << " | " << setw(11) << "CHOSEN" << " | LEXEME\n"
         << "-----------+----------------------------------------+-------------+-------\n";

    size_t p = 0;
    while (p < input.size()) {
        if (isspace((unsigned char)input[p])) { p++; continue; }

        size_t best = 0;
        int bestRule = -1;
        string cand;
        for (size_t r = 0; r < rules.size(); r++) {
            size_t n = rules[r].match(input, p);
            if (n > 0) {
                cand += rules[r].name + "=" + to_string(n) + " ";
                if (n > best) { best = n; bestRule = (int)r; }   // strict > keeps priority
            }
        }
        if (bestRule < 0) {
            cout << left << setw(10) << p << " | " << setw(38) << "(none)" << " | "
                 << setw(11) << "ERROR" << " | " << input[p] << "\n";
            p++;
            continue;
        }
        cout << left << setw(10) << p << " | " << setw(38) << cand << " | "
             << setw(11) << rules[bestRule].name << " | " << input.substr(p, best) << "\n";
        p += best;                         // consume the longest match
    }
}
