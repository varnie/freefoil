#ifndef FUNCTION_DESCRIPTOR_H_INCLUDED
#define FUNCTION_DESCRIPTOR_H_INCLUDED

#include "AST_defs.h"
#include "value_descriptor.h"
#include "param_descriptor.h"
#include "opcodes.h"
#include "runtime.h"

#include <vector>
#include <string>
#include <iostream>

#include <boost/shared_ptr.hpp>

namespace Freefoil {
    namespace Private {

        using std::string;
        using std::vector;
        using boost::shared_ptr;

        class function_descriptor {
            string name_;
            value_descriptor::E_VALUE_TYPE func_type_;
            param_descriptors_shared_ptr_list_t param_descriptors_list_;
            iter_t iter_body_;
            bool has_body_;
            Runtime::BYTE locals_count_;
        public:
            function_descriptor(const string &name, const value_descriptor::E_VALUE_TYPE func_type, const param_descriptors_shared_ptr_list_t &param_descriptors_list = param_descriptors_shared_ptr_list_t())
                    :name_(name), func_type_(func_type), param_descriptors_list_(param_descriptors_list), has_body_(false), locals_count_(0) {
            }

            const std::string &get_name() const {
                return name_;
            }
            value_descriptor::E_VALUE_TYPE get_type() const {
                return func_type_;
            }
            const param_descriptors_shared_ptr_list_t &get_param_descriptors() const {
                return param_descriptors_list_;
            }

            Runtime::BYTE get_locals_count() const{
                return locals_count_;
            }

            Runtime::BYTE get_args_count() const{
                return param_descriptors_list_.size();
            }

            bool has_body() const {
                return has_body_;
            }

            void set_body(const iter_t &iter) {
                iter_body_ = iter;
                has_body_ = true;
            }

            iter_t get_body() const {
                return iter_body_;
            }

            Runtime::BYTE get_param_descriptors_count() const {
                return param_descriptors_list_.size();
            }

            void inc_locals_count(){
                assert(locals_count_ < Runtime::max_byte_value);
                ++locals_count_;
            }
        };

        typedef shared_ptr<function_descriptor> function_shared_ptr_t;
        typedef vector<function_shared_ptr_t> function_shared_ptr_list_t;

        inline bool entry_point_functor(const function_shared_ptr_t &the_func) {
            return 	the_func->get_name() == "main";
        }
    }
}

#endif // FUNCTION_DESCRIPTOR_H_INCLUDED
