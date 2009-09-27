#ifndef FREEFOIL_VM_H_INCLUDED
#define FREEFOIL_VM_H_INCLUDED

#include "function_descriptor.h"
#include "opcodes.h"
#include <algorithm>
#include <iostream>

#include <boost/bind.hpp>

namespace Freefoil{

    using namespace Private;

    class freefoil_vm{

            typedef struct value{
                union{
                    int the_i;
                    float the_f;
                    const char *the_c;
                };
                explicit value(const int i):the_i(i){}
                explicit value(const float f):the_f(f){}
                explicit value(const char *c):the_c(c){}
            } value_t;

            typedef std::vector<value_t> stack_t;
            stack_t stack_;
            stack_t::iterator stack_top_;
            Private::function_shared_ptr_list_t funcs_;
            function_shared_ptr_t curr_executing_func_;
        public:
            freefoil_vm(){
            }

            void exec(const Private::function_shared_ptr_list_t funcs){

                funcs_ = funcs;

                function_shared_ptr_list_t::iterator iter_entrypoint
                 = std::find_if(
                        funcs_.begin(),
                        funcs_.end(),
                        boost::bind(&entry_point_functor, _1));
                assert(iter_entrypoint != funcs_.end());

                curr_executing_func_ = *iter_entrypoint;
                const Private::function_descriptor::bytecode_stream_t bytecode_stream = curr_executing_func_->get_bytecode_stream();
                Private::function_descriptor::bytecode_stream_t::const_iterator iter = bytecode_stream.begin();

                stack_top_ = stack_.begin();

                while(true){
                    if (iter >= bytecode_stream.end()){
                        break;
                    }

                    switch (*iter){
                        case OPCODE_itable_value:
                            handle_itable_value(iter);
                            break;
                        case OPCODE_ftable_value:
                            handle_ftable_value(iter);
                            break;
                        case OPCODE_stable_value:
                            handle_stable_value(iter);
                            break;
                        case OPCODE_istore:
                            handle_istore(iter);
                            break;
                        case OPCODE_fstore:
                            handle_fstore(iter);
                            break;
                        case OPCODE_sstore:
                            handle_sstore(iter);
                            break;
                        case OPCODE_iadd:
                            handle_iadd(iter);
                            break;
                        case OPCODE_isub:
                            handle_isub(iter);
                            break;
                        case OPCODE_imul:
                            handle_imul(iter);
                            break;
                        case OPCODE_idiv:
                            handle_idiv(iter);
                            break;
                        case OPCODE_fadd:
                            handle_fadd(iter);
                            break;
                        case OPCODE_fsub:
                            handle_fsub(iter);
                            break;
                        case OPCODE_fmul:
                            handle_fmul(iter);
                            break;
                        case OPCODE_fdiv:
                            handle_fdiv(iter);
                            break;
                        case OPCODE_sadd:
                            handle_sadd(iter);
                            break;


                        //TODO
                    }
                }
            }

            private:

            inline void handle_itable_value(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                std::size_t index = *++iter;
                int value = curr_executing_func_->get_int_value_from_table(index);
                stack_.push_back(value_t(value));
                ++stack_top_;
                ++iter;
            }

            inline void handle_ftable_value(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                float value = curr_executing_func_->get_float_value_from_table(*++iter);
                stack_.push_back(value_t(value));
                ++stack_top_;
                ++iter;
            }

            inline void handle_stable_value(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                const char *value = curr_executing_func_->get_string_value_from_table(*++iter);
                stack_.push_back(value_t(value));
                ++stack_top_;
                ++iter;
            }

            inline void handle_istore(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                std::size_t address = *++iter;
                stack_[address] = stack_.back();
                ++iter;
            }

            inline void handle_fstore(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                std::size_t address = *++iter;
                stack_[address] = stack_.back();
                ++iter;
            }

            inline void handle_sstore(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                std::size_t address = *++iter;
                stack_[address] = stack_.back();
                ++iter;
            }

            inline void handle_iadd(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                int value2 = pop_value().the_i;
                int value1 = pop_value().the_i;
                push_value(value_t(value1 + value2));
                ++iter;
            }

            inline void handle_isub(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                int value2 = pop_value().the_i;
                int value1 = pop_value().the_i;
                push_value(value_t(value1 - value2));
                ++iter;
            }

            inline void handle_imul(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                int value2 = pop_value().the_i;
                int value1 = pop_value().the_i;
                push_value(value_t(value1 * value2));
                ++iter;
            }

            inline void handle_idiv(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                int value2 = pop_value().the_i;
                int value1 = pop_value().the_i;
                if (value2 == 0){
                    //TODO: handle divizion by zero run-time error
                }
                push_value(value_t(value1 / value2));
                ++iter;
            }

            inline void handle_fadd(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                float value2 = pop_value().the_f;
                float value1 = pop_value().the_f;
                push_value(value_t(value1 + value2));
                ++iter;
            }

            inline void handle_fsub(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                float value2 = pop_value().the_f;
                float value1 = pop_value().the_f;
                push_value(value_t(value1 - value2));
                ++iter;
            }

            inline void handle_fmul(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                float value2 = pop_value().the_f;
                float value1 = pop_value().the_f;
                push_value(value_t(value1 * value2));
                ++iter;
            }

            inline void handle_fdiv(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                float value2 = pop_value().the_f;
                float value1 = pop_value().the_f;
                if (value2 == 0.0){
                    //TODO: handle divizion by zero run-time error
                }
                push_value(value_t(value1 / value2));
                ++iter;
            }

            inline void handle_sadd(Private::function_descriptor::bytecode_stream_t::const_iterator &iter){
                const char *value2 = pop_value().the_c;
                const char *value1 = pop_value().the_c;
                //TODO: strcat strings
                push_value(value_t(value_t("TODO: strcat strings")));
                ++iter;
            }

            inline value_t pop_value(){
                value_t v = stack_.back();
                --stack_top_;
                return v;
            }

            inline void push_value(const value_t &v){
                stack_.push_back(v);
                ++stack_top_;
            }
    };
}

#endif // FREEFOIL_VM_H_INCLUDED
