// one deliberate semantic error per marked line
int x = 5;
int x = 6;               // redeclaration
int y = 2.5;             // float -> int
bool flag = x;           // int -> bool
string s = "a";
int unused = 1;
int fresh;
int z = fresh + 1;       // used before initialization
z = undeclared + 1;      // undeclared variable
z = s - 1;               // string - int
z = x / 0;               // division by zero
z = 7.5 % 2;             // % on float
flag = !x;               // ! on int
flag = x && y;           // && on ints
s = x;                   // int -> string
if (x) { print(s); }     // int condition
while (s) { x = x - 1; } // string condition
print(z);
