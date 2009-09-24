#ifndef OPCODES_H_INCLUDED
#define OPCODES_H_INCLUDED

#include <ostream>

namespace Freefoil {
    namespace Private {

        enum OPCODE_KIND {
            OPCODE_ipush = 0x0,
            OPCODE_fpush = 0x1,
            OPCODE_spush = 0x2, //push string to the top of stack

            OPCODE_istore = 0x3, //pop the int value from the top of stack and store to the variable
            OPCODE_fstore = 0x4,
            OPCODE_sstore = 0x5,

            OPCODE_iadd = 0x6,
            OPCODE_fadd = 0x7,
            OPCODE_sadd = 0x8, //str + str

            OPCODE_negate = 0x9, //-

            OPCODE_isub = 0xa,
            OPCODE_fsub = 0xb,

            OPCODE_imul = 0xc,
            OPCODE_fmul = 0xd,

            OPCODE_idiv = 0xe,
            OPCODE_fdiv = 0xf,

            OPCODE_or = 0x10,
            OPCODE_xor = 0x11,
            OPCODE_and = 0x12,
            OPCODE_not = 0x13,

            OPCODE_eq = 0x14,
            OPCODE_neq = 0x15,
            OPCODE_leq = 0x16,//<=
            OPCODE_geq = 0x17, //>=
            OPCODE_greater = 0x18,
            OPCODE_less = 0x19,

            OPCODE_call = 0x1a,

            OPCODE_b2str,
            OPCODE_b2f,
            OPCODE_f2i,
            OPCODE_i2str,

            //TODO: add other opcodes
        };
    }
}

#endif // OPCODES_H_INCLUDED
