#ifndef OPCODES_H_INCLUDED
#define OPCODES_H_INCLUDED

namespace Freefoil {
    namespace Private {

     enum OP_KIND{
                PUSH_OP,
                POP_OP,
                ADD_OP,
                SUB_OP,
                MULT_OP,
                DIV_OP,
                EQUAL_OP,
                NOT_EQUAL_OP,
                LESS_OR_EQUAL_OP,
                LESS_OP,
                GREATER_OP,
                GREATER_OR_EQUAL_OP,
                NOT_OP,
                AND_OP,
                OR_OP
            };
    }
}

#endif // OPCODES_H_INCLUDED
