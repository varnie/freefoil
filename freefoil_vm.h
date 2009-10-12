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

        using boost::scoped_array;

        class freefoil_vm {

            struct stack_item {
                typedef union value{
                    int   i_;
                    float f_;
                    const char *pchar_;
                    value(const int i):i_(i){}
                    value(const float f):f_(f){}
                    value(const char *pchar):pchar_(pchar){}
                    value(){}
                } value_t;

                typedef enum E_TYPE {
                    int_type,
                    float_type,
                    string_type,
                } E_TYPE;
                value_t value_;
                E_TYPE type_;
            public:
                stack_item(const int i):value_(i), type_(int_type){}
                stack_item(const float f):value_(f), type_(float_type){}
                stack_item(const char *pchar):value_(pchar), type_(string_type){}
                stack_item(){}
            };

            const program_entry &program_;

            static const std::size_t STACK_SIZE = 512;

            scoped_array<ULONG> pMemory_;
            scoped_array<stack_item> pStack_;

            stack_item *sp_; //operands stack ponter
            BYTE ip_;        //current decoded instruction
            const BYTE *pc_; //current position in the instructions stream
            stack_item *fp_; //frame pointer
            ULONG *pMemory_sp_;

            void init() {
                pStack_.reset(new stack_item[STACK_SIZE]);
                sp_ = fp_ = pStack_.get() + STACK_SIZE;
                pMemory_.reset(new ULONG [STACK_SIZE]);
                pMemory_sp_ = pMemory_.get() + STACK_SIZE;
            }

            bool check_room(int size) {
                return sp_ - pStack_.get() >= size;
            }

            void push_int(const int i){
                assert(check_room(1));
                *--sp_ = stack_item(i);
            }

            void push_float(const float f){
                assert(check_room(1));
                *--sp_ = stack_item(f);
            }

            void push_string(const char *pchar){
                assert(check_room(1));
                *--sp_ = stack_item(pchar);
            }

            int pop_int(){
                assert(sp_ >= pStack_.get());
                assert(sp_->type_ == stack_item::int_type);
                return (*sp_++).value_.i_;
            }

            float pop_float(){
                assert(sp_ >= pStack_.get());
                assert(sp_->type_ == stack_item::float_type);
                return (*sp_++).value_.f_;
            }

            const char *pop_string(){
                assert(sp_ >= pStack_.get());
                assert(sp_->type_ == stack_item::string_type);
                return (*sp_++).value_.pchar_;
            }


            void push_memory(ULONG memory) {
                *--pMemory_sp_ = memory;
            }

            ULONG pop_memory() {
                return *pMemory_sp_++;
            }

        public:
            freefoil_vm(const program_entry &program) : program_(program) {
            }

            ~freefoil_vm() {
                //do nothing
            }

            void exec() {

                init();

                const function_template &entry_point_func = program_.user_funcs_[program_.entry_point_func_index_];
                pc_ = &*entry_point_func.instructions_.begin();
                const BYTE *pc_end = &*(entry_point_func.instructions_.end() - 1);
                push_memory((ULONG)pc_end);
                push_memory((ULONG)fp_);
                sp_ -= entry_point_func.locals_count_;

                //           sp_ -= 1; //1 local
                //           fp_ = sp_;

                while ((ip_ = *pc_++) != OPCODE_halt) {
                    switch (ip_) {
                    case OPCODE_call: {
                        const BYTE user_func_index = *pc_;
                        const BYTE *return_pc = ++pc_;
                        push_memory((ULONG)return_pc);
                        const stack_item *old_frame = sp_;
                        push_memory((ULONG)old_frame);
                        --sp_;
                        fp_ = sp_;
                        const function_template &f = program_.user_funcs_[user_func_index];
                        const BYTE locals_count = f.locals_count_;
                        check_room(locals_count);
                        sp_ -= locals_count; //make room for local vars
                        pc_ = &*f.instructions_.begin();    //advance pc_ to the function's first instruction
                        break;
                    }
                    case OPCODE_ret: {  //return void
                        fp_ = (stack_item *)pop_memory();
                        pc_ = (const BYTE *) pop_memory();
                        sp_ = fp_; //restore old fp_
                        break;
                    }
                    case OPCODE_iret: { //return int
                        fp_ = (stack_item *) pop_memory();
                        pc_ = (const BYTE *) pop_memory();
                        const int retv = pop_int();
                        sp_ = fp_;
                        push_int(retv);
                        break;
                    }
                    case OPCODE_fret: { //return float
                        //TODO:
                        break;
                    }
                    case OPCODE_sret: { //return string
                        //TODO:
                        break;
                    }
                    case OPCODE_iload_const: {
                        const BYTE int_constant_index = *pc_++;
                        const int value = program_.constants_pool_.get_int_value_from_table(int_constant_index);
                        push_int(value);
                        break;
                    }
                    case OPCODE_ipush: {
                        const BYTE variable_offset = *pc_++;
                        push_int((*(fp_ + variable_offset)).value_.i_);
                        break;
                    }
                    case OPCODE_iadd: {
                        const int value1 = pop_int();
                        const int value2 = pop_int();
                        push_int(value1 + value2);
                        break;
                    }
                    case OPCODE_istore: {
                        const BYTE variable_offset = *pc_++;
                        const int value = pop_int();
                        *(fp_ + variable_offset) = value;
                        printf("value: %d", value);    //debug only
                        break;
                    }
                    default: {
                        printf("wrong opcode: %d", ip_);
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
