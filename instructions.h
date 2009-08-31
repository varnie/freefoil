#ifndef INSTRUCTIONS_H_INCLUDED
#define INSTRUCTIONS_H_INCLUDED

namespace Freefoil{

    namespace Private{
            enum E_INSTRUCTION{
                E_LOAD_VAR, //load value from functions var's table by index to stack's top
                E_LOAD_INT_CONST, //load value from int/float/string function constant's table to stack's top
                E_LOAD_FLOAT_CONST,
                E_LOAD_STRING_CONST

                //TODO: add the rest
            };
    }
}


#endif // INSTRUCTIONS_H_INCLUDED
