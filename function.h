#ifndef FUNCTION_H_
#define FUNCTION_H_

#include "value.h"
#include "param.h"
#include <list>
#include <boost/shared_ptr.hpp>

namespace Freefoil {
	namespace Private {

		using std::list;
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
			function(const E_FUNCTION_TYPE func_type, const params_shared_ptr_list_t &params_list)
				:func_type_(func_type), params_list_(params_list)
			{}
			virtual ~function();
		private:
			private:
			E_FUNCTION_TYPE func_type_;
			params_shared_ptr_list_t params_list_;
		};
		
		typedef shared_ptr<function> function_shared_ptr_t;
		typedef list<function_shared_ptr_t> function_shared_ptr_list_t;
	}
}

#endif /*FUNCTION_H_*/
