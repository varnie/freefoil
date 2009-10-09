#include "compiler.h"
#include <string>
#include <iostream>

using std::string;

//TODO: parse program args
int main() {

    bool optimize, save_2_file, show, execute;
    optimize = true;
    save_2_file = false;
    show = true;
    execute = true;

    Freefoil::compiler c;

    string str;

    while (getline(std::cin, str)) {
        if (str == "q") {
            break;
        }

        c.exec(str, optimize, save_2_file, show, execute);
    }

    return 0;
}
