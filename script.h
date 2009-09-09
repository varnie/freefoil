#ifndef SCRIPT_H_
#define SCRIPT_H_

#define BOOST_SPIRIT_DUMP_PARSETREE_AS_XML 1

#include "freefoil_defs.h"
#include "function_descriptor.h"
#include "symbol_table.h"
#include "scope_stack.h"

namespace Freefoil {

    using Private::function_shared_ptr_list_t;
    using Private::function_shared_ptr_t;
    using Private::params_shared_ptr_list_t;
    using Private::param_shared_ptr_t;
    using Private::iter_t;
    using Private::symbol_table;
    using Private::scope_stack;
    using Private::function_descriptor;
    using Private::OPCODE_KIND;

    class script {

        function_shared_ptr_list_t core_funcs_list_;
        function_shared_ptr_list_t funcs_list_;
        symbol_table curr_symbol_table_;
        scope_stack curr_scope_stack_;
        function_shared_ptr_t curr_parsing_function;
        int stack_offset_;
    private:
        void parse(const iter_t &iter);
        void parse_script(const iter_t &iter);
        void parse_func_decl(const iter_t &iter);
        void parse_func_impl(const iter_t &iter);
        function_shared_ptr_t parse_func_head(const iter_t &iter);
        void parse_func_body(const iter_t &iter);
        params_shared_ptr_list_t parse_func_params_list(const iter_t &iter);
        param_shared_ptr_t parse_func_param(const iter_t &iter);
        static std::string parse_str(const iter_t &iter);
        void parse_stmt(const iter_t &iter);
        void parse_expr(const iter_t &iter);
        void parse_term(const iter_t &iter);
        void parse_factor(const iter_t &iter);
        void parse_bool_expr(const iter_t &iter);
        void parse_bool_term(const iter_t &iter);
        void parse_bool_factor(const iter_t &iter);
        void parse_bool_relation(const iter_t &iter);
        void parse_number(const iter_t &iter);
        void parse_or_tail(const iter_t &iter);
        void parse_and_tail(const iter_t &iter);
        void parse_quoted_string(const iter_t &iter);
    public:
        script();
        void exec();
    };
}

#endif /*SCRIPT_H_*/
