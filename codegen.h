#ifndef CODEGEN_H_INCLUDED
#define CODEGEN_H_INCLUDED

#include "AST_defs.h"
#include "function_descriptor.h"
#include "opcodes.h"

#include <list>
#include <vector>
#include <stack>

#include <boost/shared_ptr.hpp>

namespace Freefoil {
    namespace Private {

        using Private::iter_t;
        using std::vector;
        using std::list;
        using std::string;
        using std::stack;

        using boost::shared_ptr;

        class codegen {
        private:
            class code_chunk;
            typedef shared_ptr<code_chunk> code_chunk_shared_ptr_t;

            typedef struct code_chunk {
                function_descriptor::BYTECODE bytecode_;
                bool is_plug_;  //to be patched later
                code_chunk_shared_ptr_t jump_dst_; //if is_plug_ == true, then we must patch our bytecode_ with the address of jump_dst_->pnext_ instruction
            } code_chunk_t;

            typedef vector<code_chunk_shared_ptr_t> jumps_t;
            stack<jumps_t> true_jmps_, false_jmps_;

            typedef list<code_chunk_shared_ptr_t> bytecode_stream_t;
            typedef vector<bytecode_stream_t> bytecode_streams_t;
            bytecode_streams_t bytecode_streams_;

            function_shared_ptr_list_t parsed_funcs_;

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

            void code_emit_branch(function_descriptor::BYTECODE opcode);
            void code_emit(function_descriptor::BYTECODE opcode);
            void code_emit(function_descriptor::BYTECODE opcode, std::size_t index);
            void code_emit_cast(value_descriptor::E_VALUE_TYPE src_type, value_descriptor::E_VALUE_TYPE cast_type);
            void code_emit_plug();
            void set_jumps_dsts(vector<code_chunk_shared_ptr_t> &jumps_table, const code_chunk_shared_ptr_t &dst_code_chunk);

        public:
            codegen();
            void exec(const iter_t &tree_top, const function_shared_ptr_list_t &parsed_funcs);
        };
    }
}

#endif // CODEGEN_H_INCLUDED
