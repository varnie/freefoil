#ifndef OPCODES_H_INCLUDED
#define OPCODES_H_INCLUDED

namespace Freefoil {
    namespace Private {

        enum OPCODE_KIND {

            //variables load/store
            OPCODE_ipush = 1, //pop next offset as address, and push integer value from stack by that address
            OPCODE_fpush = 2, //pop next offset as address, and push float value from stack by that address
            OPCODE_spush = 3, //pop next offset as address, and push string value from stack by that address
            OPCODE_istore = 4, //pop the integer value from the top of stack; pop next offset and store to stacks's address by this offset
            OPCODE_fstore = 5, //pop the float value from the top of stack; pop next offset and store to stacks's address by this offset
            OPCODE_sstore = 6, //pop the string value from the top of stack; pop next offset and store to stacks's address by this offset

            //constants load
            OPCODE_iload_const = 7, //pop next offset, push integer value from integer constants pool
            OPCODE_fload_const = 8, //pop next offset, push float value from integer constants pool
            OPCODE_sload_const = 9, //pop next offset, push string value from integer constants pool

            //operations
            OPCODE_iadd = 10,
            OPCODE_fadd = 11,
            OPCODE_sadd = 12, //str + str

            OPCODE_negate = 13, //-

            OPCODE_isub = 14,
            OPCODE_fsub = 15,

            OPCODE_imul = 16,
            OPCODE_fmul = 17,

            OPCODE_idiv = 18,
            OPCODE_fdiv = 19,

            OPCODE_xor = 20,

            OPCODE_ifeq = 21,
            OPCODE_ifneq = 22,
            OPCODE_ifleq = 23,//<=
            OPCODE_ifgeq = 24, //>=
            OPCODE_ifgreater = 25,
            OPCODE_ifless = 26,

            OPCODE_call = 27,

            OPCODE_b2str = 28,
            OPCODE_b2f = 29,
            OPCODE_f2i = 30,
            OPCODE_i2str = 31,
            OPCODE_i2f = 32,

            OPCODE_true = 33, //push 1 on top of stack
            OPCODE_false = 34,//push 0 on top of stack

            OPCODE_jz = 35, /*jump if false*/
            OPCODE_jnz = 36, /*jump if true*/
            OPCODE_jmp = 37, /*unconditional goto */

            //TODO: add other opcodes
        };
    }
}

#endif // OPCODES_H_INCLUDED
