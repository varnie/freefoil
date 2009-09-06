#ifndef OPCODES_H_INCLUDED
#define OPCODES_H_INCLUDED

namespace Freefoil {
    namespace Private {

        enum OPCODE_KIND {
            PUSH_INT_CONST,
            PUSH_FLOAT_CONST,
            PUSH_STR_CONST,
            PUSH_TRUE,
            PUSH_FALSE,
            PUSH_SPACE, //space for 1 instruction
            STORE_VAR,
            //TODO: add other opcodes
        };

        struct instruction {
            OPCODE_KIND op_;
            union {
                float f_;
                int i_;
                char *pChar_;
                void *pRef_;
            };
            instruction(OPCODE_KIND op):op_(op) {}
            instruction(OPCODE_KIND op, float f):op_(op), f_(f) {}
            instruction(OPCODE_KIND op, int i):op_(op), i_(i) {}
            instruction(OPCODE_KIND op, char *pChar):op_(op), pChar_(pChar) {}
        };

    }
}

#endif // OPCODES_H_INCLUDED
