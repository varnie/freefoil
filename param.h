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

		class param{
			value_descriptor::E_VALUE_TYPE value_type_;
			bool is_ref_;
			string name_;
		public:
			param(const value_descriptor::E_VALUE_TYPE value_type, bool is_ref = false, const std::string name = string())
				:value_type_(value_type), is_ref_(is_ref), name_(name)
				{}
			const string &get_name() const{
				return name_;
			}
			const value_descriptor::E_VALUE_TYPE get_value_type() const{
				return value_type_;
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
