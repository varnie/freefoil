#ifndef FUNCTION_DESCRIPTOR_H_INCLUDED
#define FUNCTION_DESCRIPTOR_H_INCLUDED

#include "freefoil_defs.h"
#include "value_descriptor.h"
#include "param.h"
#include "opcodes.h"

#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>


namespace Freefoil {
	namespace Private {

		using std::list;
		using std::string;
		using std::vector;
		using boost::shared_ptr;

		class function_descriptor{
		public:
            typedef vector<instruction> bytecode_stream_t;
            typedef vector<int> int_table_t;
            typedef vector<float> float_table_t;
            typedef vector<string> string_table_t;

			enum E_FUNCTION_TYPE{
				intType,
				floatType,
				boolType,
				stringType,
				voidType
			};

			function_descriptor(const string &name, const E_FUNCTION_TYPE func_type, const params_shared_ptr_list_t &params_list = params_shared_ptr_list_t())
				:name_(name), func_type_(func_type), params_list_(params_list), has_body_(false)
			{
            }

			const std::string &get_name() const{
				return name_;
			}
			E_FUNCTION_TYPE get_type() const{
				return func_type_;
			}
			const params_shared_ptr_list_t &get_params() const{
				return params_list_;
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

			inline void add_instruction(const instruction &instr){
			    bytecode_stream_.push_back(instr);
			}

			std::size_t get_params_count() const{
			    return params_list_.size();
			}

			void add_int_constant(const int i){
			    if (std::count(int_table_.begin(), int_table_.end(), i) == 0){
                        int_table_.push_back(i);
			    }
			}

            void add_float_constant(const float f){
			    if (std::count(float_table_.begin(), float_table_.end(), f) == 0){
                        float_table_.push_back(f);
			    }
			}

			std::size_t get_index_of_int_constant(const int i) const{
                    assert(std::count(int_table_.begin(), int_table_.end(), i) == 1);
                    return *std::find(int_table_.begin(), int_table_.end(), i);
			}

			void print_bytecode_stream() const{
                    std::cout << "bytecode: ";
                    for (bytecode_stream_t::const_iterator iter = bytecode_stream_.begin(), iter_end = bytecode_stream_.end(); iter != iter_end; ++iter){
                        std::cout << *iter;
                    }
                    std::cout << std::endl;
			}

		private:
			string name_;
			E_FUNCTION_TYPE func_type_;
			params_shared_ptr_list_t params_list_;
			iter_t iter_body_;
			bool has_body_;

			int_table_t int_table_;
			float_table_t float_table_;
			string_table_t string_table;

            bytecode_stream_t bytecode_stream_;
		};

		typedef shared_ptr<function_descriptor> function_shared_ptr_t;
		typedef list<function_shared_ptr_t> function_shared_ptr_list_t;
	}
}

#endif // FUNCTION_DESCRIPTOR_H_INCLUDED
