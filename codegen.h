#ifndef CODEGEN_H_INCLUDED
#define CODEGEN_H_INCLUDED

#include "AST_defs.h"
#include "function_descriptor.h"
#include "opcodes.h"

#include <vector>

namespace Freefoil {
    namespace Private {

        using Private::iter_t;
        using std::vector;

        class codegen {
            function_shared_ptr_list_t parsed_funcs_;
        public:
            typedef vector<function_descriptor::bytecode_stream_t> funcs_bytecode_streams_t;
        private:

            funcs_bytecode_streams_t funcs_bytecodes_;
            std::size_t funcs_count_;

            void codegen_script(const iter_t &iter);
            void codegen_func_impl(const iter_t &iter);
            void codegen_func_body(const iter_t &iter);
            void codegen_stmt(const iter_t &iter);
            void codegen_var_declare_stmt_list(const iter_t &iter);
            void codegen_expr(const iter_t &iter);
            void codegen_term(const iter_t &iter);
            void codegen_factor(const iter_t &iter);
            void codegen_bool_expr(const iter_t &iter);
            void codegen_bool_term(const iter_t &iter);
            void codegen_bool_factor(const iter_t &iter);
            void codegen_bool_relation(const iter_t &iter);
            void codegen_number(const iter_t &iter);
            void codegen_quoted_string(const iter_t &iter);
            void codegen_bool_constant(const iter_t &iter);
            void codegen_func_call(const iter_t &iter);
            void codegen_ident(const iter_t &iter);
            void codegen_and_op(const iter_t &iter);
            void codegen_or_xor_op(const iter_t &iter);
            void codegen_cmp_op(const iter_t &iter);
            void codegen_mult_divide_op(const iter_t &iter);
            void codegen_plus_minus_op(const iter_t &iter);

            void code_emit(OPCODE_KIND opcode);
            void code_emit(OPCODE_KIND opcode, std::size_t index);
            void code_emit_cast(value_descriptor::E_VALUE_TYPE src_type, value_descriptor::E_VALUE_TYPE cast_type);

        public:
            codegen();
            void exec(const iter_t &tree_top, const function_shared_ptr_list_t &parsed_funcs);
        };
    }
}

#endif // CODEGEN_H_INCLUDED
