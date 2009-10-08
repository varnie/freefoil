#ifndef FREEFOIL_VM_H_INCLUDED
#define FREEFOIL_VM_H_INCLUDED

#include "runtime.h"
#include "opcodes.h"
#include <cassert>

#include <vector>
#include <boost/scoped_array.hpp>

namespace Freefoil {

    namespace Runtime {

        using namespace Private;
        using std::vector;
        using boost::scoped_array;

        typedef vector<BYTE> instructions_stream_t;
        typedef vector<function_template> function_templates_vector_t;
        typedef unsigned long ULONG;

        class freefoil_vm {
            function_templates_vector_t user_funcs_;

            ULONG *sp_; //operands stack ponter
            BYTE ip_;  //current decoded instruction
            BYTE *pc_; //current position in the instructions stream
            ULONG *fp_; //frame pointer
            ULONG *pMemory_sp_;

            static const std::size_t STACK_SIZE = 512;

            scoped_array<ULONG> pMemory_;
            scoped_array<ULONG> pStack_;
            instructions_stream_t instructions_;
            constants_pool constants_pool_;

            void init() {
                pStack_.reset(new ULONG[STACK_SIZE]);
                sp_ = fp_ = pStack_.get() + STACK_SIZE;
                pMemory_.reset(new ULONG [STACK_SIZE]);
                pMemory_sp_ = pMemory_.get() + STACK_SIZE;
            }

            bool check_room(int size){
                return sp_ - pStack_.get() >= size;
            }

            void push_long(ULONG value){
                assert(check_room(1));
                *--sp_ = value;
            }

            ULONG pop_long(){
                return *sp_++;
            }

            void push_memory(ULONG memory){
                *--pMemory_sp_ = memory;
            }

            ULONG pop_memory(){
                return *pMemory_sp_++;
            }

        public:
            freefoil_vm(const function_templates_vector_t &user_funcs, const instructions_stream_t &instructions, const constants_pool &constants, BYTE *pc) {
                user_funcs_ = user_funcs;
                instructions_ = instructions;
                constants_pool_ = constants;
                pc_ = pc;
                init();
            }

            ~freefoil_vm(){
                //do nothing
            }

            void exec(const function_template &entry_point){

     //           sp_ -= 1; //1 local
     //           fp_ = sp_;

                while ((ip_ = *pc_++) != OPCODE_halt){
                    switch (ip_){
                        case OPCODE_call:{
                            const BYTE user_func_index = *pc_;
                            const BYTE *return_pc = ++pc_;
                            push_memory((ULONG)return_pc);
                            const ULONG *old_frame = sp_;
                            push_memory((ULONG)old_frame);
                            --sp_;
                            fp_ = sp_;
                            const function_template f = user_funcs_[user_func_index];
                            const BYTE locals_count = f.locals_count_;
                            check_room(locals_count);
                            sp_ -= locals_count; //make room for local vars
                            pc_ = f.pc_;    //advance pc_ to the function's first instruction
                            break;
                        }
                        case OPCODE_ret:{   //return void
                            fp_ = (ULONG *)pop_memory();
                            pc_ = (BYTE *) pop_memory();
                            sp_ = fp_; //restore old fp_
                            break;
                        }
                        case OPCODE_iret:{  //return int
                            fp_ = (ULONG *) pop_memory();
                            pc_ = (BYTE *) pop_memory();
                            const ULONG retv = pop_long();
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
                            push_long(*(fp_ + variable_offset));
                            break;
                        }
                        case OPCODE_iadd:{
                            ULONG value1 = pop_long();
                            ULONG value2 = pop_long();
                            push_long(value1 + value2);
                            break;
                        }
                        case OPCODE_istore:{
                            const BYTE variable_offset = *pc_++;
                            ULONG value = pop_long();
                            *(fp_ + variable_offset) = value;
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
