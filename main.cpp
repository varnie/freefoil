#include "compiler.h"
#include "freefoil_vm.h"
#include <string>
#include <iostream>

#include <boost/shared_ptr.hpp>

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
	
	do {
		std::cout << "please enter the program or press q to exit" << std::endl;
		getline(std::cin, str);
		
		Freefoil::Runtime::program_entry_shared_ptr the_program = c.exec(str, optimize, show);
        if (the_program){

            if (save_2_file) {
                //TODO:
            }

            if (execute) {
                Freefoil::Runtime::freefoil_vm vm(*the_program.get());
                vm.exec(); //TODO: add sending params
                std::cout << std::endl;
            }
        }
	} while (str != "q");
	
    return 0;
}
