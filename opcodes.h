#ifndef OPCODES_H_INCLUDED
#define OPCODES_H_INCLUDED

#include <ostream>

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
            GET_VAR_INDEX,
            LOAD_VAR,
            OR_OP,
            XOR_OP,
            AND_OP,
            NOT_OP,
            NEGATE_OP, //-
            PLUS_OP,
            MINUS_OP,
            MULT_OP,
            DIVIDE_OP,
            //TODO: add other opcodes
        };

        struct instruction {
            friend std::ostream& operator<< ( std::ostream& theStream, instruction& the_instruction);

            OPCODE_KIND op_;
            union {
                float f_;
                int i_;
                char *pChar_;
                void *pObj_;
            };
            explicit instruction(OPCODE_KIND op):op_(op) {}
            instruction(OPCODE_KIND op, float f):op_(op), f_(f) {}
            instruction(OPCODE_KIND op, int i):op_(op), i_(i) {}
            instruction(OPCODE_KIND op, char *pChar):op_(op), pChar_(pChar) {}
            instruction(int offset):i_(offset) {}
            std::ostream & operator << (std::ostream &os) {
                os << f_;
                return os;
            }
        };

        inline std::ostream& operator<< ( std::ostream& theStream, const instruction & the_instruction){

            theStream << the_instruction.op_;
            if (the_instruction.op_ == GET_VAR_INDEX){
                theStream << the_instruction.i_;
            }

            return theStream;
        }
    }
}

#endif // OPCODES_H_INCLUDED
