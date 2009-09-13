#ifndef SEMANTIC_ANALYZER_H_INCLUDED
#define SEMANTIC_ANALYZER_H_INCLUDED

#include "AST_defs.h"
#include "function_descriptor.h"
#include "symbols_handler.h"

#include <boost/scoped_ptr.hpp>

namespace Freefoil {

    namespace Private {

        using Private::function_shared_ptr_list_t;
        using Private::function_shared_ptr_t;
        using Private::param_descriptors_shared_ptr_list_t;
        using Private::param_descriptor_shared_ptr_t;
        using Private::iter_t;
        using Private::function_descriptor;
        using Private::OPCODE_KIND;
        using Private::symbols_handler;
        using boost::scoped_ptr;

        class semantic_analyzer {

            function_shared_ptr_list_t core_funcs_list_;
            function_shared_ptr_list_t funcs_list_;

            typedef scoped_ptr<symbols_handler> symbols_handler_scoped_ptr;
            symbols_handler_scoped_ptr symbols_handler_;
            function_shared_ptr_t curr_parsing_function_;
            int stack_offset_;

            void setup_core_funcs();
            void analyze(const iter_t &iter);
            void analyze_script(const iter_t &iter);
            void analyze_func_decl(const iter_t &iter);
            void analyze_func_impl(const iter_t &iter);
            function_shared_ptr_t analyze_func_head(const iter_t &iter);
            void analyze_func_body(const iter_t &iter);
            param_descriptors_shared_ptr_list_t analyze_func_param_descriptors_list(const iter_t &iter);
            param_descriptor_shared_ptr_t analyze_func_param_descriptor(const iter_t &iter);
            static std::string analyze_str(const iter_t &iter);
            void analyze_stmt(const iter_t &iter);
            void analyze_expr(const iter_t &iter);
            void analyze_term(const iter_t &iter);
            void analyze_factor(const iter_t &iter);
            void analyze_bool_expr(const iter_t &iter);
            void analyze_bool_term(const iter_t &iter);
            void analyze_bool_factor(const iter_t &iter);
            void analyze_bool_relation(const iter_t &iter);
            void analyze_number(const iter_t &iter);
            void analyze_or_tail(const iter_t &iter);
            void analyze_and_tail(const iter_t &iter);
            void analyze_quoted_string(const iter_t &iter);
            void analyze_boolean_constant(const iter_t &iter);
            void analyze_func_call(const iter_t &iter);
        };

    }

#endif // SEMANTIC_ANALYZER_H_INCLUDED
