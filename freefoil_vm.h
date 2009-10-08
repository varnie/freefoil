#ifndef FREEFOIL_VM_H_INCLUDED
#define FREEFOIL_VM_H_INCLUDED

#include "runtime.h"
#include "opcodes.h"
#include <cassert>

#include <vector>

namespace Freefoil {

    namespace Runtime {

        using namespace Private;
        using std::vector;

        typedef Freefoil::Runtime::function_template function_template_t;
        typedef Freefoil::Runtime::freefoil_vm freefoil_vm_t;
        typedef Freefoil::Runtime::constants_pool constants_pool_t;
        typedef vector<BYTE> instructions_stream_t;
        typedef vector<function_template> function_templates_vector_t;

        class freefoil_vm {
        private:
            function_templates_vector_t user_funcs_;

            typedef long LONG;

            LONG *sp_; //stack pointer
            BYTE ip_;  //current decoded instruction
            BYTE *pc_; //current position in the instructions stream
            LONG *fp_; //frame pointer
            LONG *pMemory_sp_;

            static const std::size_t STACK_SIZE = 512;

            LONG *pMemory_;
            LONG *pStack_;
            vector<BYTE> instructions_;
            constants_pool constants_pool_;

            void init() {
                pStack_ = new LONG[STACK_SIZE];
                sp_ = fp_ = pStack_ + STACK_SIZE;
                pMemory_ = new LONG [STACK_SIZE];
                pMemory_sp_ = pMemory_ + STACK_SIZE;
            }

            void release() {
                delete [] pStack_;
                delete [] pMemory_;
            }

            bool check_room(int size){
                return sp_ - pStack_ >= size;
            }

            void push_long(LONG value){
                assert(check_room(1));
                *--sp_ = value;
            }

            LONG pop_long(){
                return *sp_++;
            }

            void push_memory(LONG memory){
                *--pMemory_sp_ = memory;
            }

            LONG pop_memory(){
                return *pMemory_sp_++;
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
                            const BYTE *return_pc = ++pc_;
                            push_memory((LONG)return_pc);
                            //const LONG *curr_frame = fp_;
                            //push_memory((LONG)curr_frame);
                            //fp_ = sp_;
                            const function_template f = user_funcs_[user_func_index];
                            const BYTE locals_count = f.locals_count_;
                            check_room(locals_count);
                            sp_ -= locals_count; //make room for local vars
                            pc_ = f.pc_;    //advance pc_ to the function's first instruction
                            break;
                        }
                        case OPCODE_ret:{   //return void
                            //*fp_ = pop_memory();
                            *pc_ = pop_memory();
                            sp_ = fp_; //restore old fp_
                            break;
                        }
                        case OPCODE_iret:{  //return int
                            const LONG retv = pop_long();
                            //fp_ = (LONG *) pop_memory();
                            pc_ = (BYTE *) pop_memory();
                            sp_ = fp_;
                            push_long(retv);
                            break;
                        }
                        case OPCODE_fret:{  //return float
                            //TODO:
                            break;
                        }
                        case OPCODE_sret:{  //return string
                            //TODO:
                            break;
                        }
                        case OPCODE_iload_const:{
                            const BYTE int_constant_index = *pc_++;
                            const int value = constants_pool_.get_int_value_from_table(int_constant_index);
                            push_long(value);
                            break;
                        }
                        case OPCODE_ipush:{
                            const BYTE variable_offset = *pc_++;
                            push_long(*(fp_ - variable_offset));
                            break;
                        }
                        case OPCODE_iadd:{
                            LONG value1 = pop_long();
                            LONG value2 = pop_long();
                            push_long(value1 + value2);
                            break;
                        }
                        case OPCODE_istore:{
                            const BYTE variable_offset = *pc_++;
                            LONG value = pop_long();
                            *(fp_ - variable_offset) = value;
                            printf("value: %ld", value);
                            break;
                        }

                    }
                }
            }

            //TODO:
        };

    }
}

#endif // FREEFOIL_VM_H_INCLUDED
