#ifndef VALUE_DESCRIPTOR_H_INCLUDED
#define VALUE_DESCRIPTOR_H_INCLUDED

#include <string>

namespace Freefoil {
    namespace Private {

        class value_descriptor {
        public:
            enum E_VALUE_TYPE {
                undefinedType,
                intType,
                floatType,
                boolType,
                stringType,
            };
        private:
            E_VALUE_TYPE value_type_;
            int stack_offset_;
        public:
            value_descriptor(const E_VALUE_TYPE value_type, const int stack_offset)
                    :value_type_(value_type), stack_offset_(stack_offset) {}
            int get_stack_offset() const {
                return stack_offset_;
            }
            E_VALUE_TYPE get_value_type() const {
                return value_type_;
            }
        };

        inline std::string type_to_string(const value_descriptor::E_VALUE_TYPE value_type){
            if (value_type == value_descriptor::undefinedType){
                return "undefined_type";
            }else if (value_type == value_descriptor::intType){
                return "int_type";
            }else if (value_type == value_descriptor::floatType){
                return "float_type";
            }else if (value_type == value_descriptor::boolType){
                return "bool_type";
            }else{
                assert(value_type == value_descriptor::stringType);
                return "string_type";
            }
        }
    }
}

#endif // VALUE_DESCRIPTOR_H_INCLUDED
