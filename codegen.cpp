#include "codegen.h"
#include "AST_defs.h"
#include "freefoil_grammar.h"
#include "value_descriptor.h"

#include <iostream>

namespace Freefoil {

    using namespace Private;

    value_descriptor::E_VALUE_TYPE get_cast(const iter_t &iter);

    codegen::codegen():funcs_count_(0) {
    }

    void codegen::exec(const iter_t &tree_top) {

        std::cout << "codegen begin" << std::endl;

        const parser_id id = tree_top->value.id();
        assert(id == freefoil_grammar::script_ID || id == freefoil_grammar::func_decl_ID || id == freefoil_grammar::func_impl_ID);
        switch (id.to_long()) {
        case freefoil_grammar::script_ID:
            codegen_script(tree_top);
            break;
        case freefoil_grammar::func_decl_ID:
            break;
        case freefoil_grammar::func_impl_ID:
            codegen_func_impl(tree_top);
            break;
        default:
            break;
        }

        std::cout << "codegen end" << std::endl;
    }

    void codegen::codegen_script(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::script_ID);

        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            if (cur_iter->value.id() == freefoil_grammar::func_impl_ID) {
                codegen_func_impl(cur_iter);
            }
        }
    }

    void codegen::codegen_func_impl(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_impl_ID);

        funcs_bytecodes_.push_back(bytecode_stream_t());
        std::cout << "user function #" << funcs_count_ << " bytecode: ";
        codegen_func_body(iter->children.begin() + 1);
        std::cout << std::endl;
        ++funcs_count_;
    }

    void codegen::codegen_func_body(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_body_ID);

        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            codegen_stmt(cur_iter);
        }
    }

    void codegen::codegen_stmt(const iter_t &iter) {

        switch (iter->value.id().to_long()) {
        case freefoil_grammar::block_ID: {
            codegen_stmt(iter->children.begin());
            break;
        }
        case freefoil_grammar::var_declare_stmt_list_ID: {
            codegen_var_declare_stmt_list(iter);
            break;
        }
        case freefoil_grammar::stmt_end_ID:
            break;
            //TODO: check for other stmts
        default:
            break;
        }
    }

    void codegen::codegen_var_declare_stmt_list(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::var_declare_stmt_list_ID);

        //
        for (iter_t cur_iter = iter->children.begin() + 1, iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {

            //codegen var declare tails
            assert(cur_iter->value.id() == freefoil_grammar::var_declare_tail_ID);
            if (cur_iter->children.begin()->value.id() == freefoil_grammar::assign_op_ID) {
                assert(cur_iter->children.begin()->children.begin()->value.id() == freefoil_grammar::ident_ID);

                if (cur_iter->children.begin()->children.begin() + 1 != cur_iter->children.begin()->children.end()) {
                    //it is an assign expr
                    codegen_bool_expr(cur_iter->children.begin()->children.begin() + 1);

                    value_descriptor::E_VALUE_TYPE cast_type = get_cast(cur_iter->children.begin()->children.begin() + 1);
                    if (cast_type != value_descriptor::undefinedType) {
                        code_emit_cast((cur_iter->children.begin()->children.begin() + 1)->value.value().get_value_type(), cast_type);
                    }

                    const node_attributes &n = cur_iter->children.begin()->children.begin()->value.value();
                    int offset = n.get_index();
                    assert(offset != -1);
                    value_descriptor::E_VALUE_TYPE ident_value_type = n.get_value_type();
                    assert(ident_value_type != value_descriptor::undefinedType);
                    if (ident_value_type == value_descriptor::boolType || ident_value_type == value_descriptor::intType) {
                        code_emit(OPCODE_istore, offset);
                    } else if (ident_value_type == value_descriptor::floatType) {
                        code_emit(OPCODE_fstore, offset);
                    } else {
                        assert(ident_value_type == value_descriptor::stringType);
                        code_emit(OPCODE_sstore, offset);
                    }
                }
            }
        }
    }

    void codegen::codegen_bool_expr(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_expr_ID);

        const parser_id id = iter->children.begin()->value.id();
        if (id == freefoil_grammar::bool_term_ID) {
            codegen_bool_term(iter->children.begin());
        } else {
            assert(id == freefoil_grammar::or_xor_op_ID);
            codegen_or_xor_op(iter->children.begin());
        }
    }

    void codegen::codegen_bool_term(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_term_ID);

        const parser_id id = iter->children.begin()->value.id();
        if (id == freefoil_grammar::bool_factor_ID) {
            codegen_bool_factor(iter->children.begin());
        } else {
            assert(parse_str(iter->children.begin()) == "and");
            codegen_and_op(iter->children.begin());
        }
    }

    void codegen::codegen_bool_relation(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_relation_ID);

        const parser_id id = iter->children.begin()->value.id();
        if (id == freefoil_grammar::expr_ID) {
            codegen_expr(iter->children.begin());
        } else {
            assert(id == freefoil_grammar::cmp_op_ID);
            codegen_cmp_op(iter->children.begin());
        }
    }

    void codegen::codegen_expr(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::expr_ID);

        const bool has_unary_plus_minus_op = iter->children.begin()->value.id() == freefoil_grammar::unary_plus_minus_op_ID;
        if (has_unary_plus_minus_op) {
            const parser_id id = (iter->children.begin() + 1)->value.id();
            if (id == freefoil_grammar::term_ID) {
                codegen_term(iter->children.begin() + 1);
            } else {
                assert(id == freefoil_grammar::plus_minus_op_ID);
                codegen_plus_minus_op(iter->children.begin() + 1);
            }
            if (parse_str(iter->children.begin()) == "-") {
                code_emit(OPCODE_negate);
            }
        } else {
            const parser_id id = iter->children.begin()->value.id();
            if (id == freefoil_grammar::term_ID) {
                codegen_term(iter->children.begin());
            } else {
                assert(id == freefoil_grammar::plus_minus_op_ID);
                codegen_plus_minus_op(iter->children.begin());
            }
        }
    }

    void codegen::codegen_factor(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::factor_ID);

        switch (iter->children.begin()->value.id().to_long()) {
        case freefoil_grammar::func_call_ID:
            codegen_func_call(iter->children.begin());
            break;
        case freefoil_grammar::ident_ID:
            codegen_ident(iter->children.begin());
            break;
        case freefoil_grammar::number_ID:
            codegen_number(iter->children.begin());
            break;
        case freefoil_grammar::quoted_string_ID:
            codegen_quoted_string(iter->children.begin());
            break;
        case freefoil_grammar::bool_constant_ID:
            codegen_bool_constant(iter->children.begin());
            break;
        case freefoil_grammar::bool_expr_ID:
            codegen_bool_expr(iter->children.begin());
            break;
        default:
            break;
        }
    }

    void codegen::codegen_bool_constant(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_constant_ID);

        if (parse_str(iter) == "true") {
            code_emit(OPCODE_ipush, 1);
        } else {
            assert(parse_str(iter) == "false");
            code_emit(OPCODE_ipush, 0);
        }
    }

    void codegen::codegen_number(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::number_ID);

        const std::string number_as_str(parse_str(iter));
        if (number_as_str.find('.') != std::string::npos) {
            //it is float value
            code_emit(OPCODE_fpush, (int) iter->value.value().get_index());
        } else {
            //it is int value
            code_emit(OPCODE_ipush, (int) iter->value.value().get_index());
        }
    }

    void codegen::codegen_quoted_string(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::quoted_string_ID);

        code_emit(OPCODE_spush, iter->value.value().get_index());
    }

    void codegen::codegen_ident(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::ident_ID);

        const node_attributes &n = iter->value.value();
        code_emit(OPCODE_ipush, n.get_index());
    }

    void codegen::codegen_func_call(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_call_ID);
        assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::invoke_args_list_ID);

        code_emit(OPCODE_call, (int) iter->value.value().get_index());

        for (iter_t cur_iter = (iter->children.begin() + 1)->children.begin(), iter_end = (iter->children.begin() + 1)->children.end(); cur_iter != iter_end; ++cur_iter) {

            assert(cur_iter->value.id() == freefoil_grammar::bool_expr_ID);
            codegen_bool_expr(cur_iter);
        }

        //TODO: make distinguish between core and user functions calls
    }

    void codegen::codegen_term(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::term_ID);

        if (iter->children.begin()->value.id() == freefoil_grammar::factor_ID) {
            codegen_factor(iter->children.begin());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::mult_divide_op_ID);
            codegen_mult_divide_op(iter->children.begin());
        }
    }

    void codegen::codegen_bool_factor(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_factor_ID);

        const bool negate = (parse_str(iter->children.begin()) == "not");
        codegen_bool_relation(negate ? iter->children.begin() + 1 : iter->children.begin());
        if (negate) {
            code_emit(OPCODE_not);
        }
    }

    void codegen::codegen_or_xor_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::or_xor_op_ID);

        if (iter->children.begin()->value.id() == freefoil_grammar::or_xor_op_ID) {
            codegen_or_xor_op(iter->children.begin());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::bool_term_ID);
            assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::bool_term_ID);

            iter_t left_iter = iter->children.begin();
            codegen_bool_term(left_iter);

            value_descriptor::E_VALUE_TYPE cast_type = get_cast(left_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
                code_emit_cast(left_value_type, cast_type);
            }

            iter_t right_iter = left_iter + 1;
            codegen_bool_term(right_iter);

            cast_type = get_cast(right_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();
                code_emit_cast(right_value_type, cast_type);
            }

            if (parse_str(iter) == "or") {
                code_emit(OPCODE_or);
            } else {
                assert(parse_str(iter) == "xor");
                code_emit(OPCODE_xor);
            }
        }
    }

    void codegen::codegen_and_op(const iter_t &iter) {

        assert(parse_str(iter) == "and");

        if (parse_str(iter->children.begin()) == "and") {
            codegen_and_op(iter->children.begin());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::bool_factor_ID);
            assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::bool_factor_ID);

            iter_t left_iter = iter->children.begin();
            codegen_bool_factor(left_iter);

            value_descriptor::E_VALUE_TYPE cast_type = get_cast(left_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
                code_emit_cast(left_value_type, cast_type);
            }

            iter_t right_iter = left_iter + 1;
            codegen_bool_factor(right_iter);

            cast_type = get_cast(right_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();
                code_emit_cast(right_value_type, cast_type);
            }

            code_emit(OPCODE_and);
        }
    }

    void codegen::codegen_plus_minus_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::plus_minus_op_ID);

        if (iter->children.begin()->value.id() == freefoil_grammar::plus_minus_op_ID) {
            codegen_plus_minus_op(iter->children.begin());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::term_ID);
            assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::term_ID);

            iter_t left_iter = iter->children.begin();
            codegen_term(left_iter);

            value_descriptor::E_VALUE_TYPE cast_type = get_cast(left_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
                code_emit_cast(left_value_type, cast_type);
            }

            iter_t right_iter = left_iter + 1;
            codegen_term(right_iter);

            cast_type = get_cast(right_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();
                code_emit_cast(right_value_type, cast_type);
            }

            value_descriptor::E_VALUE_TYPE value_type = iter->value.value().get_value_type();

            if (parse_str(iter) == "+") {

                if (value_type == value_descriptor::floatType) {
                    code_emit(OPCODE_fadd);
                } else if (value_type == value_descriptor::intType || value_type == value_descriptor::boolType) {
                    code_emit(OPCODE_iadd);
                } else if (value_type == value_descriptor::stringType) {
                    code_emit(OPCODE_sadd);
                } else {
                    assert(false);
                }
            } else {
                assert(parse_str(iter) == "-");
                if (value_type == value_descriptor::floatType) {
                    code_emit(OPCODE_fsub);
                } else if (value_type == value_descriptor::intType || value_type == value_descriptor::boolType) {
                    code_emit(OPCODE_isub);
                } else {
                    assert(false);
                }
            }
        }
    }

    void codegen::codegen_mult_divide_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::mult_divide_op_ID);

        if (iter->children.begin()->value.id() == freefoil_grammar::mult_divide_op_ID) {
            codegen_mult_divide_op(iter->children.begin());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::factor_ID);
            assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::factor_ID);

            iter_t left_iter = iter->children.begin();
            codegen_factor(left_iter);

            value_descriptor::E_VALUE_TYPE cast_type = get_cast(left_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
                code_emit_cast(left_value_type, cast_type);
            }

            iter_t right_iter = left_iter + 1;
            codegen_factor(right_iter);

            cast_type = get_cast(right_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();
                code_emit_cast(right_value_type, cast_type);
            }

            value_descriptor::E_VALUE_TYPE value_type = iter->value.value().get_value_type();

            if (parse_str(iter) == "*") {

                if (value_type == value_descriptor::floatType) {
                    code_emit(OPCODE_fmul);
                } else if (value_type == value_descriptor::intType || value_type == value_descriptor::boolType) {
                    code_emit(OPCODE_imul);
                } else {
                    assert(false);
                }
            } else {
                assert(parse_str(iter) == "/");
                if (value_type == value_descriptor::floatType) {
                    code_emit(OPCODE_fdiv);
                } else if (value_type == value_descriptor::intType || value_type == value_descriptor::boolType) {
                    code_emit(OPCODE_idiv);
                } else {
                    assert(false);
                }
            }
        }
    }


    void codegen::codegen_cmp_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::cmp_op_ID);

        if (iter->children.begin()->value.id() == freefoil_grammar::cmp_op_ID) {
            codegen_cmp_op(iter->children.begin());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::expr_ID);
            assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::expr_ID);

            iter_t left_iter = iter->children.begin();
            codegen_expr(left_iter);

            value_descriptor::E_VALUE_TYPE cast_type = get_cast(left_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
                code_emit_cast(left_value_type, cast_type);
            }

            iter_t right_iter = left_iter + 1;
            codegen_expr(right_iter);

            cast_type = get_cast(right_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();
                code_emit_cast(right_value_type, cast_type);
            }

            const std::string cmp_operation_as_str(parse_str(iter));
            if (cmp_operation_as_str == "==") {
                code_emit(OPCODE_eq);
            } else if (cmp_operation_as_str == "!=") {
                code_emit(OPCODE_neq);
            } else if (cmp_operation_as_str == "<=") {
                code_emit(OPCODE_leq);
            } else if (cmp_operation_as_str == ">=") {
                code_emit(OPCODE_geq);
            } else if (cmp_operation_as_str == "<") {
                code_emit(OPCODE_less);
            } else {
                assert(cmp_operation_as_str == ">");
                code_emit(OPCODE_greater);
            }
        }
    }

    void codegen::code_emit(OPCODE_KIND opcode) {

        funcs_bytecodes_[funcs_count_].push_back(opcode);
        std::cout << opcode << " ";
    }

    void codegen::code_emit(OPCODE_KIND opcode, std::size_t index) {

        code_emit(opcode);
        funcs_bytecodes_[funcs_count_].push_back(index);
        std::cout << "(" << index << ") ";
    }

    void codegen::code_emit_cast(value_descriptor::E_VALUE_TYPE src_type, value_descriptor::E_VALUE_TYPE cast_type) {
        //TODO
        assert(src_type != cast_type);
        if (cast_type == value_descriptor::boolType) {

        } else if (cast_type == value_descriptor::stringType) {

        } else if (cast_type == value_descriptor::floatType) {
        } else {
            assert(cast_type == value_descriptor::intType);
        }
    }

    value_descriptor::E_VALUE_TYPE get_cast(const iter_t &iter) {
        return iter->value.value().get_cast();
    }
}
