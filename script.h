#ifndef SCRIPT_H_
#define SCRIPT_H_

#define BOOST_SPIRIT_DUMP_PARSETREE_AS_XML 1

#include "freefoil_defs.h"
#include "function.h"

namespace Freefoil {
	using Private::function_shared_ptr_list_t;
	using Private::iter_t;
	
	class script{
		function_shared_ptr_list_t funcs_list;
	private:
		void parse(const iter_t &iter);
	public:
		script();
		void exec();
	};
}

#endif /*SCRIPT_H_*/
