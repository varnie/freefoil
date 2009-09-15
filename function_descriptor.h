#ifndef FUNCTION_DESCRIPTOR_H_INCLUDED
#define FUNCTION_DESCRIPTOR_H_INCLUDED

#include "AST_defs.h"
#include "value_descriptor.h"
#include "param_descriptor.h"
#include "opcodes.h"

#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <algorithm>
#include <boost/shared_ptr.hpp>


namespace Freefoil {
	namespace Private {

		using std::list;
		using std::string;
		using std::vector;
		using boost::shared_ptr;

		class function_descriptor{
		    typedef vector<int> int_table_t;
            typedef vector<float> float_table_t;
            typedef vector<string> string_table_t;
		public:
            typedef vector<instruction> bytecode_stream_t;

			enum E_FUNCTION_TYPE{
				intType,
				floatType,
				boolType,
				stringType,
				voidType
			};

			function_descriptor(const string &name, const E_FUNCTION_TYPE func_type, const param_descriptors_shared_ptr_list_t &param_descriptors_list = param_descriptors_shared_ptr_list_t())
				:name_(name), func_type_(func_type), param_descriptors_list_(param_descriptors_list), has_body_(false)
			{
            }

			const std::string &get_name() const{
				return name_;
			}
			E_FUNCTION_TYPE get_type() const{
				return func_type_;
			}
			const param_descriptors_shared_ptr_list_t &get_param_descriptors() const{
				return param_descriptors_list_;
			}

			bool has_body() const{
				return has_body_;
			}

			void set_body(const iter_t &iter){
				iter_body_ = iter;
				has_body_ = true;
			}

			iter_t get_body() const{
			    return iter_body_;
			}

			void add_instruction(const instruction &instr){
			    bytecode_stream_.push_back(instr);
			}

			std::size_t get_param_descriptors_count() const{
			    return param_descriptors_list_.size();
			}

			std::size_t add_int_constant(const int i){
			    if (std::count(int_table_.begin(), int_table_.end(), i) == 0){
                        int_table_.push_back(i);
                        return int_table_.size();
			    }else{
                    return get_index_of_int_constant(i);
			    }
			}

            std::size_t add_float_constant(const float f){
			    if (std::count(float_table_.begin(), float_table_.end(), f) == 0){
                        float_table_.push_back(f);
                        return float_table_.size();
			    }else{
			        return get_index_of_float_constant(f);
			    }
			}

			std::size_t add_string_constant(const std::string &str){
			    if (std::count(string_table_.begin(), string_table_.end(), str) == 0){
                        string_table_.push_back(str);
                        return string_table_.size();
			    }else{
			        return get_index_of_string_constant(str);
			    }
			}

			std::size_t get_index_of_string_constant(const std::string &str) const{
                    assert(std::count(string_table_.begin(), string_table_.end(), str) == 1);
                    return std::distance(string_table_.begin(), std::find(string_table_.begin(), string_table_.end(), str));
			}

			std::size_t get_index_of_float_constant(const float f) const{
                    assert(std::count(float_table_.begin(), float_table_.end(), f) == 1);
                    return std::distance(float_table_.begin(), std::find(float_table_.begin(), float_table_.end(), f));
			}

			std::size_t get_index_of_int_constant(const int i) const{
                    assert(std::count(int_table_.begin(), int_table_.end(), i) == 1);
                    return std::distance(int_table_.begin(), std::find(int_table_.begin(), int_table_.end(), i));
			}

			void print_bytecode_stream() const{
                    std::cout << "bytecode: ";
                    for (bytecode_stream_t::const_iterator cur_iter = bytecode_stream_.begin(), iter_end = bytecode_stream_.end(); cur_iter != iter_end; ++cur_iter){
                        std::cout << *cur_iter;
                    }
                    std::cout << std::endl;
			}

		private:
			string name_;
			E_FUNCTION_TYPE func_type_;
			param_descriptors_shared_ptr_list_t param_descriptors_list_;
			iter_t iter_body_;
			bool has_body_;

			int_table_t int_table_;
			float_table_t float_table_;
			string_table_t string_table_;

            bytecode_stream_t bytecode_stream_;
		};

		typedef shared_ptr<function_descriptor> function_shared_ptr_t;
		typedef list<function_shared_ptr_t> function_shared_ptr_list_t;
	}
}

#endif // FUNCTION_DESCRIPTOR_H_INCLUDED
