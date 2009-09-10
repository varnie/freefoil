#ifndef PARAM_
#define PARAM_

#include "value_descriptor.h"
#include <boost/shared_ptr.hpp>
#include <list>
#include <string>

namespace Freefoil{
	namespace Private{

		using boost::shared_ptr;
		using std::list;
		using std::string;

		class param : public value_descriptor{
			std::string name_;
			bool is_ref_;
		public:
			param(const value_descriptor::E_VALUE_TYPE value_type, const int stack_offset, const string &name = string(), bool is_ref = false)
				:value_descriptor(value_type, stack_offset), name_(name), is_ref_(is_ref)
				{}
			const string &get_name() const{
				return name_;
			}
			bool is_ref() const{
				return is_ref_;
			}
		};
		typedef shared_ptr<param> param_shared_ptr_t;
		typedef std::list<param_shared_ptr_t> params_shared_ptr_list_t;
	}
}

#endif /*PARAM_*/
