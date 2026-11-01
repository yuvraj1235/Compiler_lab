# Compiler Design Lab – Simplified C++ Solutions

All programs are plain C++ (no classes, templates or smart pointers) and compile with `g++`.
Programs 3, 10 and 11 are Flex / Bison files, as the questions ask; their actions are C++.
Every source file starts with a comment containing **the exact question**, a short idea of the
solution, and the build/run commands.

## Prerequisites
| Tool | Needed for | Check |
|------|-----------|-------|
| `g++` (GCC 5 or newer) | everything | `g++ --version` |
| Flex | programs 3, 10, 11 | `flex --version` |
| Bison | programs 10, 11 | `bison --version` |

Install on Ubuntu / Debian / WSL: `sudo apt install build-essential flex bison`
(macOS: `brew install flex bison`; Windows: use WSL or MSYS2).

## Programs and how to run them
Open a terminal in this folder.

| # | Program | What it does | Build and run |
|---|---------|--------------|---------------|
| 1 | `01_two_buffer_input.cpp` | Two-buffer input with `lexemeBegin` / `fwd` pointers and sentinels; finds keywords and identifiers | `g++ -o two_buffer 01_two_buffer_input.cpp` then `./two_buffer sample_input.src` |
| 2 | `02_scanner.cpp` | Scanner for a mini-C subset, token table, skips comments/whitespace, reports errors with line numbers | `g++ -o scanner 02_scanner.cpp` then `./scanner sample_input.src` (or `sample_errors.src`) |
| 3 | `03_token_lexer.l` | Flex program: prints token type and lexeme | `flex -o lex.yy.cc 03_token_lexer.l` then `g++ lex.yy.cc -o lexer` then `./lexer sample_input.src` |
| 4 | `04_maximal_munch.cpp` | Longest-match rule with a trace of all candidate rules | `g++ -o munch 04_maximal_munch.cpp` then `./munch` or `./munch "if (x<=y) z<<=2;"` |
| 5 | `05_cfg_generator.cpp` | CFG and language `(aab)^n aa` for `aabaabaa` | `g++ -o cfg 05_cfg_generator.cpp` then `./cfg` |
| 6 | `06_recursive_descent_ast.cpp` | Recursive-descent parser, AST, evaluates `id + id * num` and `(id - num) / id` | `g++ -o rdp 06_recursive_descent_ast.cpp` then `./rdp` or `./rdp "a * (b + 3)"` |
| 7 | `07_ll1_parser.cpp` | FIRST/FOLLOW, LL(1) table, table-driven parser for `S→aABe, A→bAc\|d, B→f\|ε` | `g++ -o ll1 07_ll1_parser.cpp` then `./ll1` or `./ll1 abdcfe` |
| 8 | `08_left_recursion_removal.cpp` | Removes left recursion from the E, T, F grammar | `g++ -o lrr 08_left_recursion_removal.cpp` then `./lrr` |
| 9 | `09_lr0_parser.cpp` | LR(0) item sets, table, and shift/reduce parse of `abb` | `g++ -o lr0 09_lr0_parser.cpp` then `./lr0` or `./lr0 abbb` |
| 10 | `10_bison_config/` | Bison + Flex checker/evaluator for a config language with variables | see below |
| 11 | `11_bison_parentheses/` | Bison + Flex parentheses-balance and structure validator | see below |
| 12 | `12_three_address_code.cpp` | Parser, Three-Address Code, Quadruples, Triples | `g++ -o tac 12_three_address_code.cpp` then `./tac "a = b + c * d"` (or `./tac` for interactive mode) |
| 13 | `13_semantic_analyzer/` | Semantic analyzer: type checking and error detection on an AST | see below |

Programs 7 and 9 use one character per symbol (upper case = non-terminal, lower case = terminal,
`#` = ε) to keep the code short. The grammar arrays at the top of each file can be edited.

### 10. Bison + Flex configuration language
```bash
cd 10_bison_config
bison -d -o config.tab.cc config.y
flex -o lex.yy.cc config.l
g++ config.tab.cc lex.yy.cc -o config
./config sample_ok.cfg
./config sample_err.cfg
```
Language: `name = expression;` and `print expression;`, with `+ - * / %`, parentheses and `#` comments.

### 11. Bison + Flex parentheses validator
```bash
cd 11_bison_parentheses
bison -d -o paren.tab.cc paren.y
flex -o lex.yy.cc paren.l
g++ paren.tab.cc lex.yy.cc -o paren
./paren < sample_input.txt
```
For each input line it reports balanced / unbalanced parentheses, maximum nesting depth,
malformed structure (`()`, `3 + * 4`, `2(3)`), division by zero, and the value of valid lines.

### 13. Semantic analyzer
```bash
cd 13_semantic_analyzer
g++ -o semantic semantic_analyzer.cpp
./semantic sample_valid.mc       # passes
./semantic sample_errors.mc      # 12 errors + 3 warnings with line numbers
```
It prints the type-annotated AST, the symbol table and the report. Errors: undeclared variable,
redeclaration, type mismatch (`int`→`float` allowed, `float`→`int` not), invalid operand types,
non-bool conditions, division/modulo by zero. Warnings: use before initialization, unused variable.
Exit code: 0 = passed, 3 = semantic errors, 2 = syntax error.

## Notes for the viva
* **Program 1:** a lexeme must be shorter than one buffer half (16 bytes here); this is a limit of the two-buffer scheme.
* **Program 9:** for `S → A`, `A → A b | a`, state I2 = `{S → A., A → A.b}` has a shift/reduce conflict
  in a pure LR(0) table. The program shows that table, then reduces only on FOLLOW(S) = {$} (SLR style),
  which gives the usual textbook table, and parses `abb` with it.
* **Program 5:** the input `aabaabaa` is read as `(aab)² aa`, giving `S → aab A`, `A → aab A | aa`.

## Troubleshooting
* `config.tab.hh: No such file` – run `bison -d -o ...` before `flex` and `g++`, inside the program's own folder.
* If `g++` complains about C++11 features (very old compilers), add `-std=c++11`.
