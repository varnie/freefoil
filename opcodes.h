#ifndef OPCODES_H_INCLUDED
#define OPCODES_H_INCLUDED

namespace Freefoil {
    namespace Private {

        enum OPCODE_KIND {

            //variables load/store
            OPCODE_iload = 1, //pop next offset as address, and push integer value from stack by that address
            OPCODE_fload = 2, //pop next offset as address, and push float value from stack by that address
            OPCODE_sload = 3, //pop next offset as address, and push string value from stack by that address
            OPCODE_isave = 4, //pop the integer value from the top of stack; pop next offset and store to stacks's address by this offset
            OPCODE_fsave = 5, //pop the float value from the top of stack; pop next offset and store to stacks's address by this offset
            OPCODE_ssave = 6, //pop the string value from the top of stack; pop next offset and store to stacks's address by this offset

            //constants load
            OPCODE_iload_const = 7, //pop next offset, push integer value from integer constants pool
            OPCODE_fload_const = 8, //pop next offset, push float value from integer constants pool
            OPCODE_sload_const = 9, //pop next offset, push string value from integer constants pool

            //operations
            OPCODE_iadd = 10,
            OPCODE_fadd = 11,
            OPCODE_sadd = 12, //str + str

            OPCODE_inegate = 13, //-
            OPCODE_fnegate = 14,  //-

            OPCODE_isub = 15,
            OPCODE_fsub = 16,

            OPCODE_imul = 17,
            OPCODE_fmul = 18,

            OPCODE_idiv = 19,
            OPCODE_fdiv = 20,

            OPCODE_xor = 21,

            OPCODE_ifeq = 22,
            OPCODE_ifneq = 23,
            OPCODE_ifleq = 24,//<=
            OPCODE_ifgeq = 25, //>=
            OPCODE_ifgreater = 26,
            OPCODE_ifless = 27,

            OPCODE_call = 28,

            OPCODE_b2str = 29,
            OPCODE_b2f = 30,
            OPCODE_f2i = 31,
            OPCODE_i2str = 32,
            OPCODE_i2f = 33,

            OPCODE_push_true = 34, //push 1 on top of stack
            OPCODE_push_false = 35,//push 0 on top of stack

            OPCODE_jz = 36, /*jump if false*/
            OPCODE_jnz = 37, /*jump if true*/
            OPCODE_jmp = 38, /*unconditional goto */

            OPCODE_halt = 39,  //exit
            OPCODE_ret = 40,   //return
            OPCODE_iret = 41,  //return int
            OPCODE_fret = 42,  //return float
            OPCODE_sret = 43,  //return str

            OPCODE_builtin_call = 44,
            //TODO: add other opcodes
        };
    }
}

#endif // OPCODES_H_INCLUDED
