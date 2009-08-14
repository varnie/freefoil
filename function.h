#ifndef FUNCTION_H_
#define FUNCTION_H_

#include "value.h"
#include "param.h"
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>
#include "freefoil_defs.h"

namespace Freefoil {
	namespace Private {

		using std::list;
		using std::string;
		using boost::shared_ptr;

		class function{
		public:
			enum E_FUNCTION_TYPE{
				intType,
				floatType,
				boolType,
				stringType,
				voidType
			};

			function(const string &name, const E_FUNCTION_TYPE func_type, const params_shared_ptr_list_t &params_list = params_shared_ptr_list_t())
				:name_(name), func_type_(func_type), params_list_(params_list), has_body_(false)
			{}
			virtual ~function(){}
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

		private:
			string name_;
			E_FUNCTION_TYPE func_type_;
			params_shared_ptr_list_t params_list_;
			iter_t iter_body_;
			bool has_body_;
		};

		typedef shared_ptr<function> function_shared_ptr_t;
		typedef list<function_shared_ptr_t> function_shared_ptr_list_t;
	}
}

#endif /*FUNCTION_H_*/
