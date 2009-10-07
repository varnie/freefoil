#ifndef FREEFOIL_VM_H_INCLUDED
#define FREEFOIL_VM_H_INCLUDED

#include "runtime.h"
#include "opcodes.h"

#include <vector>

namespace Freefoil {

    namespace Runtime {

        using namespace Private;

        class freefoil_vm {

            typedef struct value {
                union {
                    int the_i;
                    float the_f;
                    const char *the_c;
                };
                enum t {
                    int_type,
                    float_type,
                    char_type,
                };
                t the_type;

                explicit value(const int i):the_i(i), the_type(int_type) {}
                explicit value(const float f):the_f(f), the_type(float_type) {}
                explicit value(const char *c):the_c(c), the_type(char_type) {}

                value operator - () {
                    assert(the_type == int_type or the_type == float_type);
                    return the_type == int_type ? value(- the_i) : value( - the_f);
                }
            } value_t;

            typedef vector<function_template> function_templates_vector_t;
            function_templates_vector_t user_funcs_;

            BYTE *sp_; //stack pointer
            BYTE ip_;  //current decoded instruction
            BYTE *pc_; //current position in the instructions stream
            BYTE *fp_; //frame pointer

            static const std::size_t STACK_SIZE = 512;

            BYTE *pStack_;
            vector<BYTE> instructions_;
            constants_pool constants_pool_;

            void init() {
                pStack_ = new BYTE[STACK_SIZE];
                sp_ = fp_ = pStack_ + STACK_SIZE;
            }

            void release() {
                delete [] pStack_;
            }

            bool check_room(int size){
                return sp_ - pStack_ >= size;
            }

            void push_byte(BYTE byte){
                assert(check_room(1));
                *--sp_ = byte;
            }

            void pop_byte(){
                ++sp_;
            }

            void pop_byte(BYTE &dst){
                dst = *sp_++;
            }

        public:
            freefoil_vm(const function_templates_vector_t &user_funcs, const vector<BYTE> &instructions, const constants_pool& constants, BYTE *pc) {
                user_funcs_ = user_funcs;
                instructions_ = instructions;
                constants_pool_ = constants;
                pc_ = pc;
                init();
            }

            ~freefoil_vm(){
                release();
            }

            void exec(){

                while ((ip_ = *pc_++) != OPCODE_halt){
                    switch (ip_){
                        case OPCODE_call:{
                            const BYTE user_func_index = *pc_;
                            const BYTE return_pc = *++pc_;
                            push_byte(return_pc); //push return address
                            const BYTE curr_frame = *fp_;
                            push_byte(curr_frame); //push old frame pointer
                            fp_ = sp_;
                            const function_template f = user_funcs_[user_func_index];
                            const BYTE locals_count = f.locals_count_;
                            check_room(locals_count);
                            sp_ -= locals_count; //make room for local vars
                            pc_ = f.pc_;    //advance pc_ to the function's instruction
                            break;
                        }
                        case OPCODE_ret:{   //return void
                            sp_ = fp_;
                            pop_byte(*fp_);
                            sp_ = fp_;
                            pop_byte(*pc_);
                            break;
                        }
                        /*case OPCODE_iret:{  //return int
                        }
                        case OPCODE_fret:{  //return float
                        }
                        case OPCODE_sret:{  //return string
                        }*/

                    }
                }
            }

            //TODO:
        };

    }
}

#endif // FREEFOIL_VM_H_INCLUDED
