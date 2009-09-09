#ifndef OPCODES_H_INCLUDED
#define OPCODES_H_INCLUDED

#include <ostream>

namespace Freefoil {
    namespace Private {

        enum OPCODE_KIND {
            GET_INT_CONST,
            GET_INDEX_OF_CONST,
            GET_FLOAT_CONST,
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
            EQUAL_OP,
            NOT_EQUAL_OP,
            LESS_OP,
            GREATER_OP,
            LESS_OR_EQUAL_OP,
            GREATER_OR_EQUAL_OP,
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
            explicit instruction(OPCODE_KIND op, float f):op_(op), f_(f) {}
            explicit instruction(OPCODE_KIND op, int i):op_(op), i_(i) {}
            explicit instruction(OPCODE_KIND op, char *pChar):op_(op), pChar_(pChar) {}
            explicit instruction(int offset):i_(offset) {}
        };

        inline std::ostream& operator<< ( std::ostream& theStream, const instruction & the_instruction){

            theStream << the_instruction.op_ << " ";
            if (the_instruction.op_ == GET_VAR_INDEX){
                theStream << "offset ";
                theStream << the_instruction.i_ << " ";

            }else if (the_instruction.op_ == GET_INDEX_OF_CONST){
                theStream << "index of const ";
                theStream << the_instruction.i_ << " ";

            }

            return theStream;
        }
    }
}

#endif // OPCODES_H_INCLUDED
