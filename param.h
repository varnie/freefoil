#ifndef PARAM_
#define PARAM_

#include "value.h"
#include <boost/shared_ptr.hpp>
#include <list>
#include <string>

namespace Freefoil{
	namespace Private{
		using boost::shared_ptr;
		using std::list;
		using std::string;
		
		class param{
			value::E_VALUE_TYPE value_type_;
			bool is_ref_;
			string name_;
		public:
			param(const value::E_VALUE_TYPE value_type, bool is_ref = false, const std::string name = string())
				:value_type_(value_type), is_ref_(is_ref), name_(name)
				{}
			//TODO:
		};
		typedef shared_ptr<param> param_shared_ptr_t;
		typedef  std::list<param_shared_ptr_t> params_shared_ptr_list_t;
	}
}

#endif /*PARAM_*/
