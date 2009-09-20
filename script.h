#ifndef SCRIPT_H_
#define SCRIPT_H_

#define BOOST_SPIRIT_DUMP_PARSETREE_AS_XML 1

#include "AST_defs.h"
#include "function_descriptor.h"
#include "symbols_handler.h"
#include "value_descriptor.h"

#include <boost/scoped_ptr.hpp>

namespace Freefoil {

    using Private::function_shared_ptr_list_t;
    using Private::function_shared_ptr_t;
    using Private::param_descriptors_shared_ptr_list_t;
    using Private::param_descriptor_shared_ptr_t;
    using Private::iter_t;
    using Private::function_descriptor;
    using Private::OPCODE_KIND;
    using Private::symbols_handler;
    using Private::value_descriptor;
    using boost::scoped_ptr;

    class script {

        std::size_t errors_count_;

        function_shared_ptr_list_t core_funcs_list_;
        function_shared_ptr_list_t funcs_list_;

        typedef scoped_ptr<symbols_handler> symbols_handler_scoped_ptr;
        symbols_handler_scoped_ptr symbols_handler_;
        function_shared_ptr_t curr_parsing_function_;
        int stack_offset_;

        void setup_core_funcs();
        void parse(const iter_t &iter);
        void parse_script(const iter_t &iter);
        void parse_func_decl(const iter_t &iter);
        void parse_func_impl(const iter_t &iter);
        function_shared_ptr_t parse_func_head(const iter_t &iter);
        void parse_func_body(const iter_t &iter);
        param_descriptors_shared_ptr_list_t parse_func_param_descriptors_list(const iter_t &iter);
        param_descriptor_shared_ptr_t parse_func_param_descriptor(const iter_t &iter);
        void parse_stmt(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_expr(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_term(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_factor(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_bool_expr(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_bool_term(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_bool_factor(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_bool_relation(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_number(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_or_tail(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_and_tail(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_quoted_string(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_bool_constant(const iter_t &iter);
        value_descriptor::E_VALUE_TYPE  parse_func_call(const iter_t &iter);
        void print_error(const iter_t &iter, const std::string &msg);
        void print_error(const std::string &msg);
    public:
        script();
        void exec();
    };
}

#endif /*SCRIPT_H_*/
