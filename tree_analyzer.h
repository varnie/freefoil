#ifndef TREE_ANALYZER_H_INCLUDED
#define TREE_ANALYZER_H_INCLUDED

#include "AST_defs.h"
#include "function_descriptor.h"
#include "symbols_handler.h"
#include "value_descriptor.h"
#include "runtime.h"

#include <boost/scoped_ptr.hpp>

namespace Freefoil {

    using Private::function_shared_ptr_list_t;
    using Private::function_shared_ptr_t;
    using Private::param_descriptors_t;
    using Private::param_descriptor;
    using Private::iter_t;
    using Private::function_descriptor;
    using Private::OPCODE_KIND;
    using Private::symbols_handler;
    using Private::value_descriptor;
    using Runtime::constants_pool;
    using boost::scoped_ptr;

    namespace Private {

        class tree_analyzer {

            std::size_t errors_count_, warnings_count_;

            function_shared_ptr_list_t funcs_list_, builtin_funcs_list_;
            function_shared_ptr_t curr_parsing_function_;

            typedef symbols_handler<value_descriptor> descriptors_handler_t;
            typedef scoped_ptr<descriptors_handler_t> descriptors_handler_scoped_ptr;
            descriptors_handler_scoped_ptr descriptors_handler_;

            constants_pool constants_pool_;

            Runtime::BYTE args_count_, locals_count_;

            void setup_builtin_funcs();

            void parse_script(const iter_t &iter);
            void parse_func_decl(const iter_t &iter);
            void parse_func_impl(const iter_t &iter);
            function_shared_ptr_t parse_func_head(const iter_t &iter);
            void parse_func_body(const iter_t &iter);
            param_descriptors_t parse_func_param_descriptors_list(const iter_t &iter);
            param_descriptor parse_func_param_descriptor(const iter_t &iter);
            void parse_stmt(const iter_t &iter);
            void parse_var_declare_stmt_list(const iter_t &iter);
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
            void parse_bool_constant(const iter_t &iter);
            void parse_func_call(const iter_t &iter);
            void parse_ident(const iter_t &iter);
            void parse_and_op(const iter_t &iter);
            void parse_or_xor_op(const iter_t &iter);
            void parse_cmp_op(const iter_t &iter);
            void parse_mult_divide_op(const iter_t &iter);
            void parse_plus_minus_op(const iter_t &iter);
            void parse_return_stmt(const iter_t &iter);
            void parse_if_stmt(const iter_t &iter);
            void parse_block(const iter_t &iter);
            bool has_complete_returns(const iter_t &iter);
            std::ptrdiff_t find_function(const std::string &call_name, const std::vector<value_descriptor::E_VALUE_TYPE> &invoke_args, const function_shared_ptr_list_t &funcs) const;
            static void print_error(const iter_t &iter, const std::string &msg);
            static void print_error(const std::string &msg);

        public:
            tree_analyzer();
            bool parse(const iter_t &tree_top);
            const function_shared_ptr_list_t &get_parsed_funcs_list() const;
            const constants_pool &get_parsed_constants_pool() const;
        };
    }
}

#endif // TREE_ANALYZER_H_INCLUDED
