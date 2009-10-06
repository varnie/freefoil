#include "compiler.h"
#include <string>
#include <iostream>

using std::string;

int main() {

    Freefoil::compiler c;

    string str;

    while (getline(std::cin, str)) {
        if (str == "q") {
            break;
        }

        c.exec(str);
    }

    return 0;
}
