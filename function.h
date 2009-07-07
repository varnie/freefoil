#ifndef FUNCTION_H_
#define FUNCTION_H_

#include "value.h"
#include <list>
#include <boost/shared_ptr.hpp>

namespace Freefoil {
	namespace Private {

		using std::list;
		using boost::shared_ptr;

		class function{
			class Arg{
			};
			typedef  std::list<Arg> args_t;
		public:
			enum E_FUNCTION_TYPE{
				intType,
				floatType,
				boolType,
				stringType,
				voidType
			};
			
			function(const E_FUNCTION_TYPE func_type, const args_t &args_list)
				:func_type_(func_type), args_list_(args_list)
			{}
			virtual ~function();
		private:
			private:
			E_FUNCTION_TYPE func_type_;
			args_t args_list_;
		};
		
		typedef shared_ptr<function> function_shared_ptr_t;
		typedef list<function_shared_ptr_t> function_shared_ptr_list_t;
	}
}

#endif /*FUNCTION_H_*/
