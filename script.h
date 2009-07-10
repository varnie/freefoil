#ifndef SCRIPT_H_
#define SCRIPT_H_

#define BOOST_SPIRIT_DUMP_PARSETREE_AS_XML 1

#include "freefoil_defs.h"
#include "function.h"

namespace Freefoil {
	using Private::function_shared_ptr_list_t;
	using Private::params_shared_ptr_list_t;
	using Private::param_shared_ptr_t;
	using Private::iter_t;
	
	class script{
		function_shared_ptr_list_t funcs_list;
	private:
		void parse(const iter_t &iter);
		void parse_script(const iter_t &iter);
		void parse_func_decl(const iter_t &iter);
		void parse_func_impl(const iter_t &iter);
		void parse_func_head(const iter_t &iter);
		void parse_func_body(const iter_t &iter);
		void parse_func_params_list(const iter_t &iter);
		param_shared_ptr_t parse_func_param(const iter_t &iter);
		static std::string parse_str(const iter_t &iter);
	public:
		script();
		void exec();
	};
}

#endif /*SCRIPT_H_*/
