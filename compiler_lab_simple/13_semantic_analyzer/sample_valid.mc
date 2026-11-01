// A correct program: type checking passes
int a = 10;
float rate = 2.5;
bool big = a > 5;
string name = "lab";
float total = a * rate;        // int * float -> float
while (a > 0) {
    a = a - 3;
    if (big && a < 4) {
        print(name + "!");
    } else {
        print(a);
    }
}
print(total);
