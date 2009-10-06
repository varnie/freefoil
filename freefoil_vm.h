#ifndef FREEFOIL_VM_H_INCLUDED
#define FREEFOIL_VM_H_INCLUDED

#include "function_descriptor.h"
#include "opcodes.h"

#include <iostream>

#include <boost/bind.hpp>

namespace Freefoil{

    using namespace Private;

    class freefoil_vm{

            typedef struct value{
                union{
                    int the_i;
                    float the_f;
                    const char *the_c;
                };
                enum t{
                    int_type,
                    float_type,
                    char_type,
                };
                t the_type;

                explicit value(const int i):the_i(i), the_type(int_type){}
                explicit value(const float f):the_f(f), the_type(float_type){}
                explicit value(const char *c):the_c(c), the_type(char_type){}

                value operator - (){
                    assert(the_type == int_type or the_type == float_type);
                    return the_type == int_type ? value(- the_i) : value( - the_f);
                }
            } value_t;


        public:
            freefoil_vm(){
            }

            //TODO:
    };
}

#endif // FREEFOIL_VM_H_INCLUDED
