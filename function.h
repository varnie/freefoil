#ifndef FUNCTION_H_
#define FUNCTION_H_

#include "value.h"
#include <list>

namespace Freefoil {
	namespace Private {

class function{
	typedef  std::list<Arg> args_t;
public:
	enum E_FUNCTION_TYPE{
		intType,
		floatType,
		boolType,
		stringType,
		voidType
	};
	
	function(E_FUNCTION_TYPE func_type, args_t const &args_list)
		:func_type_(func_type), args_list_(args_list)
	{}
	virtual ~function();
};
	}
private:
	E_FUNCTION_TYPE func_type_;
	args_t args_list_;
}

#endif /*FUNCTION_H_*/
