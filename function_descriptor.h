#ifndef FUNCTION_DESCRIPTOR_H_INCLUDED
#define FUNCTION_DESCRIPTOR_H_INCLUDED

#include <vector>
#include <list>
#include <string>
#include "freefoil_defs.h"
#include "value_descriptor.h"
#include "param.h"
#include <boost/shared_ptr.hpp>

namespace Freefoil {
	namespace Private {

		using std::list;
		using std::string;
		using std::vector;
		using boost::shared_ptr;

		class function_descriptor{
		public:
			enum E_FUNCTION_TYPE{
				intType,
				floatType,
				boolType,
				stringType,
				voidType
			};

			function_descriptor(const string &name, const E_FUNCTION_TYPE func_type, const params_shared_ptr_list_t &params_list = params_shared_ptr_list_t())
				:name_(name), func_type_(func_type), params_list_(params_list), has_body_(false)
			{}

			const std::string &get_name() const{
				return name_;
			}
			const E_FUNCTION_TYPE get_type() const{
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

		private:
			string name_;
			E_FUNCTION_TYPE func_type_;
			params_shared_ptr_list_t params_list_;
			iter_t iter_body_;
			bool has_body_;
            typedef vector<size_t> bytecode_stream;
            bytecode_stream bytecode_stream_;
		};

		typedef shared_ptr<function_descriptor> function_shared_ptr_t;
		typedef list<function_shared_ptr_t> function_shared_ptr_list_t;
	}
}

#endif // FUNCTION_DESCRIPTOR_H_INCLUDED
