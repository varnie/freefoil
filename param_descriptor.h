#ifndef param_descriptor_
#define param_descriptor_

#include "AST_defs.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>

namespace Freefoil{
	namespace Private{

		using boost::shared_ptr;
		using std::vector;
		using std::string;

		class param_descriptor : public value_descriptor{
			std::string name_;
			bool is_ref_;
		public:
			param_descriptor(const E_VALUE_TYPE value_type, const int stack_offset, const string &name = string(), bool is_ref = false)
				:value_descriptor(value_type, stack_offset), name_(name), is_ref_(is_ref)
				{}
			const string &get_name() const{
				return name_;
			}
			bool is_ref() const{
				return is_ref_;
			}
		};
		typedef shared_ptr<param_descriptor> param_descriptor_shared_ptr_t;
		typedef vector<param_descriptor_shared_ptr_t> param_descriptors_shared_ptr_list_t;
	}
}

#endif /*param_descriptor_*/
