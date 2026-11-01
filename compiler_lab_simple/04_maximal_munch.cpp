/*
 * QUESTION:
 * 2) Write a program to implement a lexical analyzer component that applies
 *    the Maximal Munch (Longest Match) rule to resolve tokenization
 *    ambiguities.
 *
 * IDEA
 *   At every position all token rules are tried; each tells how many
 *   characters it can match. The LONGEST match wins. On a tie the rule
 *   listed first wins (KEYWORD before IDENTIFIER).
 *     "iffy"  -> IDENTIFIER (4 chars) beats KEYWORD "if" (2 chars)
 *     "<<="   -> one operator, not  <  <  =
 *     "12.5"  -> FLOAT, not INT . INT
 *
 * Build : g++ -o munch 04_maximal_munch.cpp
 * Run   : ./munch                        (built-in test string)
 *         ./munch "if (x<=y) z<<=2;"     (your own string)
 */
#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>
using namespace std;

string keywords[] = {"if", "else", "while", "for", "int", "return"};
string operators[] = {"<<=", ">>=", "==", "!=", "<=", ">=", "<<", ">>", "++", "--",
                      "+=", "-=", "->", "&&", "||", "=", "<", ">", "+", "-", "*", "/", "!"};
string ruleName[] = {"KEYWORD", "IDENTIFIER", "FLOAT", "INTEGER", "OPERATOR", "DELIMITER"};

// length of the longest word of list[] that s has at position p (0 = none)
int longestMatch(string s, int p, string list[], int n) {
    int best = 0;
    for (int i = 0; i < n; i++)
        if ((int)list[i].size() > best && s.compare(p, list[i].size(), list[i]) == 0)
            best = list[i].size();
    return best;
}

int digitsAt(string s, int p) {
    int n = 0;
    while (p + n < (int)s.size() && isdigit(s[p + n])) n++;
    return n;
}

// how many characters does rule r match at position p ?
int matchRule(int r, string s, int p) {
    if (r == 0) return longestMatch(s, p, keywords, 6);
    if (r == 1) {
        if (!(isalpha(s[p]) || s[p] == '_')) return 0;
        int n = 0;
        while (p + n < (int)s.size() && (isalnum(s[p + n]) || s[p + n] == '_')) n++;
        return n;
    }
    if (r == 2) {
        int n = digitsAt(s, p);
        if (n > 0 && p + n < (int)s.size() && s[p + n] == '.') {
            int f = digitsAt(s, p + n + 1);
            if (f > 0) return n + 1 + f;
        }
        return 0;
    }
    if (r == 3) return digitsAt(s, p);
    if (r == 4) return longestMatch(s, p, operators, 23);
    return string("(){}[];,.").find(s[p]) != string::npos ? 1 : 0;
}

int main(int argc, char *argv[]) {
    string input = argc > 1 ? argv[1] : "iffy if ifx==y a<<=b x>>=2 a+++b 12.5.3 p->q while1 while 7";
    cout << "Input: \"" << input << "\"\n\n";
    cout << left << setw(5) << "POS" << setw(38) << "CANDIDATE RULES (match length)"
         << setw(12) << "CHOSEN" << "LEXEME\n" << string(65, '-') << "\n";

    int p = 0;
    while (p < (int)input.size()) {
        if (isspace(input[p])) { p++; continue; }
        int best = 0, bestRule = -1;
        string candidates = "";
        for (int r = 0; r < 6; r++) {
            int n = matchRule(r, input, p);
            if (n > 0) {
                candidates += ruleName[r] + "=" + to_string(n) + " ";
                if (n > best) { best = n; bestRule = r; }    // '>' keeps the earlier rule on a tie
            }
        }
        if (bestRule == -1) {
            cout << setw(5) << p << setw(38) << "(none)" << setw(12) << "ERROR" << input[p] << "\n";
            p++;
        } else {
            cout << setw(5) << p << setw(38) << candidates << setw(12) << ruleName[bestRule]
                 << input.substr(p, best) << "\n";
            p += best;                                       // consume the longest match
        }
    }
}
