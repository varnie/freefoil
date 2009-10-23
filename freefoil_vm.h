#ifndef FREEFOIL_VM_H_INCLUDED
#define FREEFOIL_VM_H_INCLUDED

#include "runtime.h"
#include "opcodes.h"
#include "memory_manager.h"
#include "exceptions.h"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <map>

#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>

namespace Freefoil {

    namespace Runtime {

        using namespace Private;

        using boost::scoped_array;
        using boost::shared_ptr;

        using std::map;
        using std::vector;
        using std::string;

        typedef memory_manager::gcobject_instance_t gcobject_instance_t;

        extern memory_manager g_mm;

        class freefoil_vm {

//static const int i = 1;
//#define is_bigendian() ( (*(char*)&i) == 0 )

            union stack_item {
                stack_item *pstack_item_;
                gcobject_instance_t gcobj_;
                int   i_;
                float f_;
            };

            typedef void (freefoil_vm::*builtin_func_pointer_t)();

            typedef struct builtin_func {
                std::size_t args_count_;
                builtin_func_pointer_t body_;
                builtin_func(const std::size_t args_count, builtin_func_pointer_t body)
                    :args_count_(args_count), body_(body)
                    {}
            } builtin_func_t;

            vector<builtin_func_t> builtin_funcs_;

            const program_entry &program_;

            static const std::size_t STACK_SIZE = 512;

            scoped_array<ULONG> pMemory_;
            scoped_array<stack_item> pStack_;

            stack_item *sp_; //operands stack ponter
            BYTE ip_;        //current decoded instruction
            const BYTE *pc_; //current position in the instructions stream
            stack_item *fp_; //frame pointer
            ULONG *pMemory_sp_;

            //bool is_big_endian;

            void print_int(){
                std::cout << pop_int();
            }

            void print_float(){
                std::cout << pop_float();
            }

            void print_string(){
                std::cout << *pop_gcobject();
            }

            void init() {
                pStack_.reset(new stack_item[STACK_SIZE]);
                sp_ = fp_ = pStack_.get() + STACK_SIZE;
                pMemory_.reset(new ULONG[STACK_SIZE]);
                pMemory_sp_ = pMemory_.get() + STACK_SIZE;

                builtin_funcs_.push_back(builtin_func_t(1, &freefoil_vm::print_int));
                builtin_funcs_.push_back(builtin_func_t(1, &freefoil_vm::print_float));
                builtin_funcs_.push_back(builtin_func_t(1, &freefoil_vm::print_int)); //print_bool == print_int
                builtin_funcs_.push_back(builtin_func_t(1, &freefoil_vm::print_string));
                //TODO: add others
            }

            bool check_room(int size) {
                return sp_ - pStack_.get() >= size;
            }

            void push_int(const int i) {
                assert(check_room(1));
                (*--sp_).i_ = i;
            }

            void push_float(const float f) {
                assert(check_room(1));
                (*--sp_).f_ = f;
            }

            void push_gcobject(const gcobject_instance_t &gcobj) {
                assert(check_room(1));
                (*--sp_).gcobj_ = gcobj;
            }

            int pop_int() {
                assert(sp_ >= pStack_.get());
//                assert(sp_->type_ == int_type);
                return (*sp_++).i_;
            }

            float pop_float() {
                assert(sp_ >= pStack_.get());
//                assert(sp_->type_ == float_type);
                return (*sp_++).f_;
            }

            gcobject_instance_t pop_gcobject() {
                assert(sp_ >= pStack_.get());
                //assert(sp_->type_ == stack_item::string_type);
                return (*sp_++).gcobj_;
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
                push_memory(program_.args_count_);
                push_memory((ULONG)pc_end); //return pc
                push_memory((ULONG)fp_); //old fp

                //TODO: arguments

                sp_ -= entry_point_func.locals_count_;

                g_mm.function_begin();

                try {
                    while ((ip_ = *pc_++) != OPCODE_halt) {
                        switch (ip_) {

                        case OPCODE_builtin_call: {
                            const builtin_func_t &builtin_func = builtin_funcs_[*pc_++];

                            g_mm.function_begin();
                            (this->*builtin_func.body_)();
                            sp_ += builtin_func.args_count_;
                            g_mm.function_end();
                            break;
                        }

                        case OPCODE_call: {
                            const BYTE user_func_index = *pc_;
                            const function_template &f = program_.user_funcs_[user_func_index];

                            push_memory(f.args_count_);

                            const BYTE *return_pc = ++pc_;
                            push_memory((ULONG)return_pc);

                            const stack_item *old_frame = sp_;
                            push_memory((ULONG)old_frame);

                            --sp_;
                            fp_ = sp_;

                            check_room(f.locals_count_);
                            sp_ -= f.locals_count_; //make room for local vars
                            pc_ = &*f.instructions_.begin();    //advance pc_ to the function's first instruction

                            g_mm.function_begin();
                            break;
                        }

                        case OPCODE_ret: {  //return void
                            fp_ = (stack_item *)pop_memory();
                            pc_ = (const BYTE *) pop_memory();
                            sp_ = fp_; //restore old fp_

                            sp_ += pop_memory(); //args count

                            g_mm.function_end();
                            break;
                        }

                        case OPCODE_iret: { //return int
                            fp_ = (stack_item *) pop_memory();
                            pc_ = (const BYTE *) pop_memory();
                            const int retv = pop_int();
                            sp_ = fp_;

                            sp_ += pop_memory(); //args count

                            push_int(retv);

                            g_mm.function_end();
                            break;
                        }

                        case OPCODE_fret: { //return float
                            fp_ = (stack_item *) pop_memory();
                            pc_ = (const BYTE *) pop_memory();
                            const float retv = pop_float();
                            sp_= fp_;

                            sp_ += pop_memory(); //args count

                            push_float(retv);

                            g_mm.function_end();
                            break;
                        }

                        case OPCODE_sret: { //return string
                            fp_ = (stack_item *) pop_memory();
                            pc_ = (const BYTE *) pop_memory();
                            const gcobject_instance_t gcobj = pop_gcobject();
                            sp_ = fp_;

                            sp_ += pop_memory(); //args count

                            push_gcobject(gcobj);

                            g_mm.function_end();
                            break;
                        }

                        case OPCODE_iload_const: {
                            const BYTE int_constant_index = *pc_++;
                            const int value = program_.constants_pool_.get_int_value_from_table(int_constant_index);
                            push_int(value);
                            break;
                        }

                        case OPCODE_fload_const: {
                            const BYTE float_constant_index = *pc_++;
                            const float value = program_.constants_pool_.get_float_value_from_table(float_constant_index);
                            push_float(value);
                            break;
                        }

                        case OPCODE_sload_const: {
                            const BYTE string_constant_index = *pc_++;
                            const std::string &value = program_.constants_pool_.get_string_value_from_table(string_constant_index);
                            gcobject_instance_t gcobj = g_mm.sload(value);
                            push_gcobject(gcobj);
                            break;
                        }

                        case OPCODE_iload: {
                            const BYTE variable_offset = *pc_++;
                            push_int((*(fp_ + variable_offset)).i_);
                            break;
                        }

                        case OPCODE_isave: {
                            const int value = pop_int();
                            const BYTE variable_offset = *pc_++;
                            (*(fp_ + variable_offset)).i_ = value;
                            break;
                        }

                        case OPCODE_fsave: {
                            const float value = pop_float();
                            const BYTE variable_offset = *pc_++;
                            (*(fp_ + variable_offset)).i_ = value;
                            break;
                        }

                        case OPCODE_ssave: {
                            const gcobject_instance_t gcobj = pop_gcobject();
                            const BYTE variable_offset = *pc_++;
                            (*(fp_ + variable_offset)).gcobj_ = gcobj;
                            break;
                        }

                        case OPCODE_fadd: {
                            const float value2 = pop_float();
                            push_float(pop_float() + value2);
                            break;
                        }

                        case OPCODE_fmul: {
                            const float value2 = pop_float();
                            push_float(pop_float() * value2);
                            break;
                        }

                        case OPCODE_fdiv: {
                            const float value2 = pop_float();
                            if (value2 == 0.0) {
                                throw freefoil_exception("runtime exception: divizion by zero");
                            }
                            push_float(pop_float() / value2);
                            break;
                        }

                        case OPCODE_fsub: {
                            const float value2 = pop_float();
                            push_float(pop_float() - value2);
                            break;
                        }

                        case OPCODE_iadd: {
                            const int value2 = pop_int();
                            push_int(pop_int() + value2);
                            break;
                        }

                        case OPCODE_imul: {
                            const int value2 = pop_int();
                            push_int(pop_int() * value2);
                            break;
                        }

                        case OPCODE_idiv: {
                            const int value2 = pop_int();
                            if (value2 == 0) {
                                throw freefoil_exception("runtime exception: divizion by zero");
                            }
                            push_float((float) (pop_int() / value2));
                            break;
                        }

                        case OPCODE_isub: {
                            const int value2 = pop_int();
                            push_int(pop_int() - value2);
                            break;
                        }

                        case OPCODE_sload: {
                            const BYTE variable_offset = *pc_++;
                            push_gcobject((*(fp_ + variable_offset)).gcobj_);
                            break;
                        }

                        case OPCODE_sadd: {
                             //TODO
                            break;
                        }

                        case OPCODE_inegate: {
                            const int value = pop_int();
                            push_int(- value);
                            break;
                        }

                        case OPCODE_fnegate: {
                            const float value = pop_float();
                            push_float(- value);
                            break;
                        }

                        case OPCODE_f2i: {
                            //warning: possibly, information lost
                            push_int(static_cast<int>(pop_float()));
                            break;
                        }

                        case OPCODE_i2f: {
                            push_float(pop_int());
                            break;
                        }

                        case OPCODE_jmp: {
                            const BYTE relative_offset = *pc_;
                            pc_ += relative_offset;
                            break;
                        }

                        case OPCODE_jnz: {  //jmp if true
                            const int value = pop_int();
                            assert(value == 0 or value == 1);
                            if (value == 1){
                                pc_ += *pc_;
                            }else{
                                ++pc_;
                            }
                            break;
                        }

                        case OPCODE_jz: { //jmp if false
                            const int value = pop_int();
                            assert(value == 0 or value == 1);
                            if (value == 0){
                                pc_ += *pc_;
                            }else{
                                ++pc_;
                            }
                            break;
                        }

                        case OPCODE_push_true: {
                            push_int(1);
                            break;
                        };

                        case OPCODE_push_false: {
                            push_int(0);
                            break;
                        }

                        case OPCODE_ifeq: {
                            if (pop_int() == pop_int()){
                                pc_ += *pc_;
                            }else{
                                ++pc_;
                            }
                            break;
                        }

                        case OPCODE_ifneq: {
                            if (pop_int() != pop_int()){
                                pc_ += *pc_;
                            }else{
                                ++pc_;
                            }
                            break;
                        };

                        case OPCODE_ifgreater: {
                            if (pop_int() < pop_int()){
                                pc_ += *pc_;
                            }else{
                                ++pc_;
                            }
                            break;
                        }

                        case OPCODE_ifless: {
                            if (pop_int() > pop_int()){
                                pc_ += *pc_;
                            }else{
                                ++pc_;
                            }
                            break;
                        }

                        case OPCODE_ifgeq: {
                            if (pop_int() <= pop_int()){
                                pc_ += *pc_;
                            }else{
                                ++pc_;
                            }
                            break;
                        }

                         case OPCODE_ifleq: {
                            if (pop_int() >= pop_int()){
                                pc_ += *pc_;
                            }else{
                                ++pc_;
                            }
                            break;
                        }

                        //TODO:

                        default: {
                            throw freefoil_exception("wrong opcode: " + ip_);
                        }
                        }
                    }
                } catch (const std::exception &e) {
                    std::cout << e.what() << std::endl;
                } catch (...) {
                    std::cout << "unknown exception" << std::endl;
                }

                g_mm.function_end();
                //TODO: g_mm.dealloc();
            }
        };

    }

}

#endif // FREEFOIL_VM_H_INCLUDED
