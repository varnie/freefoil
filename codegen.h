#ifndef CODEGEN_H_INCLUDED
#define CODEGEN_H_INCLUDED

#include "AST_defs.h"
#include "function_descriptor.h"
#include "runtime.h"

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

            typedef struct codechunk {
                Runtime::BYTE bytecode_;
                int instruction_position_;  //absolute position, [0, ...)
                bool is_plug_;  //to be patched later
                const codechunk *jump_dst_; //if is_plug_ == true, then we must patch our bytecode_ with the address of an instruction following after the jump_dst_
            } codechunk_t;

            typedef shared_ptr<codechunk> codechunk_shared_ptr_t;
            typedef vector<codechunk_shared_ptr_t> codechunks_pool_t;
            codechunks_pool_t codechunks_pool_;

            typedef list<codechunk_t *> codechunk_list_t;
            typedef vector<codechunk_list_t> codechunks_t;

            codechunks_t codechunks_;
            function_shared_ptr_list_t user_funcs_;
            Runtime::ULONG entry_point_func_index_;

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
            void codegen_return_stmt(const iter_t &iter);
            void codegen_if_stmt(const iter_t &iter);
            void codegen_block(const iter_t &iter);
            void code_emit_branch(Runtime::BYTE opcode);
            void code_emit(Runtime::BYTE opcode);
            void code_emit(Runtime::BYTE opcode, Runtime::BYTE index);
            void code_emit_cast(value_descriptor::E_VALUE_TYPE src_type, value_descriptor::E_VALUE_TYPE cast_type);
            void code_emit_plug();
            void set_jumps_dsts(vector<codechunk_t *> &jumps_table, const codechunk_t *dst_code_chunk);
            void set_jmp_dst(codechunk_t *codechunk, const codechunk_t *dst_codechunk);
            void resolve_jumps();
            Runtime::program_entry_shared_ptr generate_program_entry(const Runtime::constants_pool &constants, bool show) const;
        public:
            codegen();
            Runtime::program_entry_shared_ptr exec(const iter_t &tree_top, const function_shared_ptr_list_t &user_funcs, const Runtime::constants_pool &constants, bool optimize, bool show);
        };
    }
}

#endif // CODEGEN_H_INCLUDED
