#include "codegen.h"
#include "AST_defs.h"
#include "freefoil_grammar.h"
#include "value_descriptor.h"
#include "opcodes.h"

#include <map>
#include <iostream>

#include <boost/bind.hpp>

namespace Freefoil {

    using namespace Private;

    value_descriptor::E_VALUE_TYPE get_cast(const iter_t &iter) {
        return iter->value.value().get_cast();
    }

    codegen::codegen() {
    }

    void codegen::resolve_jumps() {

        typedef std::multimap<codechunk_t *, codechunk_t *> codechunk_map_t;
        codechunk_map_t dst2srcmap;

        std::size_t instruction_index = 0;

        for (codechunks_t::const_iterator cur_user_func_iter = codechunks_.begin(), user_func_iter_end = codechunks_.end();
                cur_user_func_iter != user_func_iter_end;
                ++cur_user_func_iter
            ) {
            for (codechunk_list_t::const_iterator codechunk_begin_iter = (*cur_user_func_iter).begin(), curr_codechunk_iter = codechunk_begin_iter, codechunk_iter_end = (*cur_user_func_iter).end();
                    curr_codechunk_iter != codechunk_iter_end;
                    ++curr_codechunk_iter) {

                codechunk_t *curr_codechunk = *curr_codechunk_iter;
                curr_codechunk->instruction_position_ = instruction_index;

                if (!curr_codechunk->is_plug_) {
                    //is it a target for any codechunks?
                    std::pair<codechunk_map_t::iterator, codechunk_map_t::iterator> itp = dst2srcmap.equal_range(curr_codechunk);
                    for (codechunk_map_t::iterator it = itp.first; it != itp.second; ++it) {
                        codechunk_t *src_to_be_patched = it->second;
                        src_to_be_patched->bytecode_ = instruction_index - src_to_be_patched->instruction_position_;
                    }
                } else {
                    //it is a plug. it needs to be backpatched
                    codechunk_list_t::const_iterator iter = std::find(codechunk_begin_iter, codechunk_iter_end, curr_codechunk->jump_dst_);
                    assert(iter != codechunk_iter_end);

                    codechunk_t *jump_dst =  *(++iter);
                    dst2srcmap.insert(std::make_pair<codechunk_t *, codechunk_t *>(jump_dst, curr_codechunk));
                }

                ++instruction_index;
            }
        }
    }

    Runtime::program_entry_shared_ptr codegen::generate_program_entry(const Runtime::constants_pool &constants, bool show) const {

        assert(user_funcs_.size() == codechunks_.size());

        Runtime::function_templates_vector_t user_funcs_templates;
        user_funcs_templates.reserve(user_funcs_.size());

        if (show) {
            std::cout << "bytecode for compiled user functions:" << std::endl;
        }
        std::size_t function_index = 0;
        for (codechunks_t::const_iterator cur_user_func_iter = codechunks_.begin(), user_func_iter_end = codechunks_.end();
                cur_user_func_iter != user_func_iter_end;
                ++cur_user_func_iter
            ) {
            Runtime::instructions_stream_t instructions;
            instructions.reserve((*cur_user_func_iter).size());
            for (codechunk_list_t::const_iterator curr_codechunk_iter = (*cur_user_func_iter).begin(), codechunk_iter_end = (*cur_user_func_iter).end();
                    curr_codechunk_iter != codechunk_iter_end;
                    ++curr_codechunk_iter
                ) {
                instructions.push_back((*curr_codechunk_iter)->bytecode_);
                if (show) {
                    std::cout << (int) (*curr_codechunk_iter)->bytecode_ << " ";
                }
            }
            const function_shared_ptr_t &user_func = user_funcs_[function_index];
            user_funcs_templates.push_back(Runtime::function_template(user_func->get_args_count(), user_func->get_locals_count(), instructions, user_func->get_type() == value_descriptor::voidType));
            ++function_index;
        }
	if (show) {
	    std::cout << std::endl;
	}

        return Runtime::program_entry_shared_ptr(new Runtime::program_entry(user_funcs_templates, constants, entry_point_func_index_));
    }

    Runtime::program_entry_shared_ptr codegen::exec(const iter_t &tree_top, const function_shared_ptr_list_t &user_funcs, const Runtime::constants_pool &constants, bool optimize, bool show) {

        std::cout << "codegen begin" << std::endl;

        codechunks_.clear();
        user_funcs_ = user_funcs;
        function_shared_ptr_list_t::const_iterator entry_point_func_iter = std::find_if(
                    user_funcs.begin(),
                    user_funcs.end(),
                    boost::bind(&entry_point_functor, _1));
        assert(entry_point_func_iter != user_funcs.end());
        entry_point_func_index_ = std::distance(user_funcs.begin(), entry_point_func_iter);

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

        if (optimize) {
            //TODO:
        }

        resolve_jumps();

        std::cout << "codegen end" << std::endl;

        return generate_program_entry(constants, show);
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

        codechunks_.push_back(codechunk_list_t());
        codegen_func_body(iter->children.begin() + 1);

        assert(iter->value.value().get_value_type() != value_descriptor::undefinedType);
        if (iter->value.value().get_value_type() == value_descriptor::voidType) {
            if (codechunks_.back().empty() or codechunks_.back().back()->bytecode_ != OPCODE_ret) {
                code_emit(OPCODE_ret);
            }
        }

        if (codechunks_.size() - 1 == entry_point_func_index_) {
            code_emit(OPCODE_halt);
        }
    }

    void codegen::codegen_func_body(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_body_ID);

        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            codegen_stmt(cur_iter);
        }
    }

    void codegen::codegen_block(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::block_ID);

        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            codegen_stmt(cur_iter);
        }
    }

    void codegen::codegen_stmt(const iter_t &iter) {

        switch (iter->value.id().to_long()) {
        case freefoil_grammar::block_ID: {
            codegen_block(iter);
            break;
        }
        case freefoil_grammar::var_declare_stmt_list_ID: {
            codegen_var_declare_stmt_list(iter);
            break;
        }
        case freefoil_grammar::return_stmt_ID: {
            codegen_return_stmt(iter);
            break;
        }
        case freefoil_grammar::stmt_end_ID: {
            break;
        }
        case freefoil_grammar::func_call_ID: {
            codegen_func_call(iter);
            break;
        }
        case freefoil_grammar::if_stmt_ID: {
            codegen_if_stmt(iter);
            break;
        }
        //TODO: check for other stmts
        default:
            break;
        }
    }

    void codegen::codegen_if_stmt(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::if_stmt_ID);

        //true_jmps_.push(jumps_t());
        vector<codechunk_t *> true_jmps;

        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            if (cur_iter->value.id() != freefoil_grammar::else_branch_ID) {
                assert(cur_iter->value.id() == freefoil_grammar::if_branch_ID or cur_iter->value.id() == freefoil_grammar::elsif_branch_ID);

                //false_jmps_.push(jumps_t());

                codegen_bool_expr(cur_iter->children.begin());

                code_emit_branch(OPCODE_jz /*jump if false*/);
                //false_jmps_.top().push_back(codechunks_.back().back());
                codechunk_t *false_jmp_to_be_backpatched = codechunks_.back().back();

                codegen_block(cur_iter->children.begin() + 1);

                code_emit_branch(OPCODE_jmp /*unconditional jump*/);
                //true_jmps_.top().push_back(codechunks_.back().back());
                true_jmps.push_back(codechunks_.back().back());

                //set_jumps_dsts(false_jmps_.top(), codechunks_.back().back());
                set_jmp_dst(false_jmp_to_be_backpatched, codechunks_.back().back());

                //false_jmps_.pop();
            } else {
                codegen_block(cur_iter->children.begin());
            }
        }

        //set_jumps_dsts(true_jmps_.top(), code_chunks_.back().back());
        set_jumps_dsts(true_jmps, codechunks_.back().back());

        //true_jmps_.pop();
    }

    void codegen::codegen_return_stmt(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::return_stmt_ID);

        if (!iter->children.empty()) {
            assert(iter->children.size() == 1);
            assert(iter->children.begin()->value.id() == freefoil_grammar::bool_expr_ID);
            codegen_bool_expr(iter->children.begin());

            value_descriptor::E_VALUE_TYPE val_type;

            value_descriptor::E_VALUE_TYPE cast_type = get_cast(iter->children.begin());
            if (cast_type != value_descriptor::undefinedType) {
                code_emit_cast(iter->children.begin()->value.value().get_value_type(), cast_type);
                val_type = cast_type;
            } else {
                val_type = iter->children.begin()->value.value().get_value_type();
            }

            if (val_type == value_descriptor::intType or val_type == value_descriptor::boolType) {
                code_emit(OPCODE_iret);
            } else if (val_type == value_descriptor::floatType) {
                code_emit(OPCODE_fret);
            } else {
                assert(val_type == value_descriptor::stringType);
                code_emit(OPCODE_sret);
            }
        } else {
            code_emit(OPCODE_ret);
        }
    }

    void codegen::codegen_var_declare_stmt_list(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::var_declare_stmt_list_ID);

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
                    value_descriptor::E_VALUE_TYPE ident_value_type = n.get_value_type();
                    assert(ident_value_type != value_descriptor::undefinedType);
                    if (ident_value_type == value_descriptor::boolType || ident_value_type == value_descriptor::intType) {
                        code_emit(OPCODE_isave, offset);
                    } else if (ident_value_type == value_descriptor::floatType) {
                        code_emit(OPCODE_fsave, offset);
                    } else {
                        assert(ident_value_type == value_descriptor::stringType);
                        code_emit(OPCODE_ssave, offset);
                    }
                }
            }
        }
    }

    void codegen::codegen_bool_expr(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_expr_ID);

        //true_jmps_.push(jumps_t());
        //false_jmps_.push(jumps_t());

        const parser_id id = iter->children.begin()->value.id();
        if (id == freefoil_grammar::bool_term_ID) {
            codegen_bool_term(iter->children.begin());
        } else {
            assert(id == freefoil_grammar::or_xor_op_ID);
            codegen_or_xor_op(iter->children.begin());
        }

        //true_jmps_.pop();
        //false_jmps_.pop();
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
        const iter_t cur_iter = has_unary_plus_minus_op ? iter->children.begin() + 1 : iter->children.begin();

        const parser_id id = cur_iter->value.id();
        if (id == freefoil_grammar::term_ID) {
            codegen_term(cur_iter);
        } else {
            assert(id == freefoil_grammar::plus_minus_op_ID);
            codegen_plus_minus_op(cur_iter);
        }
        if (has_unary_plus_minus_op && parse_str(cur_iter - 1) == "-") {
            if (iter->value.value().get_value_type() == value_descriptor::floatType) {
                code_emit(OPCODE_fnegate);
            } else {
                assert(iter->value.value().get_value_type() == value_descriptor::intType);
                code_emit(OPCODE_inegate);
            }
        }

        /*
        if (has_unary_plus_minus_op) {
            const parser_id id = (iter->children.begin() + 1)->value.id();
            if (id == freefoil_grammar::term_ID) {
                codegen_term(iter->children.begin() + 1);
            } else {
                assert(id == freefoil_grammar::plus_minus_op_ID);
                codegen_plus_minus_op(iter->children.begin() + 1);
            }
            if (parse_str(iter->children.begin()) == "-") {
                if (iter->value.value().get_value_type() == value_descriptor::floatType){
                    code_emit(OPCODE_fnegate);
                }else{
                    assert(iter->value.value().get_value_type() == value_descriptor::intType);
                    code_emit(OPCODE_inegate);
                }
            }
        } else {
            const parser_id id = iter->children.begin()->value.id();
            if (id == freefoil_grammar::term_ID) {
                codegen_term(iter->children.begin());
            } else {
                assert(id == freefoil_grammar::plus_minus_op_ID);
                codegen_plus_minus_op(iter->children.begin());
            }
        }*/
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
        case freefoil_grammar::bool_expr_in_parenthesis_ID:
            codegen_bool_expr(iter->children.begin());
            break;
        default:
            break;
        }
    }

    void codegen::codegen_bool_constant(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_constant_ID);

        if (parse_str(iter) == "true") {
            code_emit(OPCODE_push_true);
        } else {
            assert(parse_str(iter) == "false");
            code_emit(OPCODE_push_false);
        }
    }

    void codegen::codegen_number(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::number_ID);

        const std::string number_as_str(parse_str(iter));
        if (number_as_str.find('.') != std::string::npos) {
            //it is float value
            code_emit(OPCODE_fload_const, iter->value.value().get_index());
        } else {
            //it is int value
            code_emit(OPCODE_iload_const, iter->value.value().get_index());
        }
    }

    void codegen::codegen_quoted_string(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::quoted_string_ID);

        code_emit(OPCODE_sload_const, iter->value.value().get_index());
    }

    void codegen::codegen_ident(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::ident_ID);

        const node_attributes &n = iter->value.value();
        value_descriptor::E_VALUE_TYPE var_type = n.get_value_type();
        if (var_type == value_descriptor::boolType or var_type == value_descriptor::intType) {
            code_emit(OPCODE_iload, n.get_index());
        } else if (var_type == value_descriptor::floatType) {
            code_emit(OPCODE_fload, n.get_index());
        } else {
            assert(var_type == value_descriptor::stringType);
            code_emit(OPCODE_sload, n.get_index());
        }
    }

    void codegen::codegen_func_call(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_call_ID);
        assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::invoke_args_list_ID);

        for (iter_t cur_iter = (iter->children.begin() + 1)->children.end() - 1, iter_end = (iter->children.begin() + 1)->children.begin(); cur_iter >= iter_end; --cur_iter) {
            assert(cur_iter->value.id() == freefoil_grammar::bool_expr_ID);
            codegen_bool_expr(cur_iter);
        }

        if (iter->value.value().get_func_kind() == node_attributes::BUILTIN_FUNC){
            code_emit(OPCODE_builtin_call, iter->value.value().get_index());
        }else{
            assert(iter->value.value().get_func_kind() == node_attributes::USER_FUNC);
            code_emit(OPCODE_call, iter->value.value().get_index());
        }
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

            //         true_jmps_.push(jumps_t());
            //         false_jmps_.push(jumps_t());

            code_emit_branch(OPCODE_jnz /*jump if true*/);
            //          false_jmps_.top().push_back(code_chunks_.back().back());
            codechunk_t *true_jmp_to_be_backpatched = codechunks_.back().back();

            code_emit(OPCODE_push_true);
            code_emit_branch(OPCODE_jmp /*unconditional jump*/);
            //          true_jmps_.top().push_back(code_chunks_.back().back());
            codechunk_t *jmp_to_be_backpatched = codechunks_.back().back();

            //          set_jumps_dsts(false_jmps_.top(), code_chunks_.back().back());
            set_jmp_dst(true_jmp_to_be_backpatched, jmp_to_be_backpatched);

            code_emit(OPCODE_push_false);
            //         set_jumps_dsts(true_jmps_.top(), code_chunks_.back().back());
            set_jmp_dst(jmp_to_be_backpatched, codechunks_.back().back());

            //         true_jmps_.pop();
            //         false_jmps_.pop();
        }
    }

    void codegen::codegen_or_xor_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::or_xor_op_ID);

        iter_t left_iter = iter->children.begin();
        iter_t right_iter = left_iter + 1;

        if (left_iter->value.id() == freefoil_grammar::or_xor_op_ID) {
            codegen_or_xor_op(left_iter);
        } else {
            codegen_bool_term(left_iter);
        }

        value_descriptor::E_VALUE_TYPE cast_type = get_cast(left_iter);
        if (cast_type != value_descriptor::undefinedType) {
            value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
            code_emit_cast(left_value_type, cast_type);
        }

        if (parse_str(iter) == "or") {

            code_emit_branch(OPCODE_jnz /*jump if true*/);
            //true_jmps_.top().push_back(code_chunks_.back().back());
            codechunk_t *true_jmp_to_be_backpatched = codechunks_.back().back();

            codegen_bool_term(right_iter);


            cast_type = get_cast(right_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();
                code_emit_cast(right_value_type, cast_type);
            }

            //set_jumps_dsts(true_jmps_.top(), code_chunks_.back().back());
            set_jmp_dst(true_jmp_to_be_backpatched, codechunks_.back().back());
        } else {
            assert(parse_str(iter) == "xor");

            codegen_bool_term(right_iter);

            cast_type = get_cast(right_iter);
            if (cast_type != value_descriptor::undefinedType) {
                value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();
                code_emit_cast(right_value_type, cast_type);
            }

            code_emit(OPCODE_xor);
        }
    }

    void codegen::codegen_and_op(const iter_t &iter) {

        assert(parse_str(iter) == "and");

        iter_t left_iter = iter->children.begin();
        iter_t right_iter = left_iter + 1;

        if (parse_str(left_iter) == "and") {
            codegen_and_op(left_iter);
        } else {
            codegen_bool_factor(left_iter);
        }

        value_descriptor::E_VALUE_TYPE cast_type = get_cast(left_iter);
        if (cast_type != value_descriptor::undefinedType) {
            value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
            code_emit_cast(left_value_type, cast_type);
        }

        code_emit_branch(OPCODE_jz /*jump if false*/);
        //false_jmps_.top().push_back(code_chunks_.back().back());
        codechunk_t *false_jmp_to_be_backpatched = codechunks_.back().back();

        codegen_bool_factor(right_iter);

        cast_type = get_cast(right_iter);
        if (cast_type != value_descriptor::undefinedType) {
            value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();
            code_emit_cast(right_value_type, cast_type);
        }

        //set_jumps_dsts(false_jmps_.top(), code_chunks_.back().back());
        set_jmp_dst(false_jmp_to_be_backpatched, codechunks_.back().back());
    }

    void codegen::codegen_plus_minus_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::plus_minus_op_ID);

        iter_t left_iter = iter->children.begin();
        iter_t right_iter = left_iter + 1;

        if (left_iter->value.id() == freefoil_grammar::plus_minus_op_ID) {
            codegen_plus_minus_op(left_iter);
        } else {
            codegen_term(left_iter);
        }

        value_descriptor::E_VALUE_TYPE cast_type = get_cast(left_iter);
        if (cast_type != value_descriptor::undefinedType) {
            value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
            code_emit_cast(left_value_type, cast_type);
        }

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

    void codegen::codegen_mult_divide_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::mult_divide_op_ID);

        iter_t left_iter = iter->children.begin();
        iter_t right_iter = left_iter + 1;

        if (left_iter->value.id() == freefoil_grammar::mult_divide_op_ID) {
            codegen_mult_divide_op(left_iter);
        } else {
            codegen_factor(left_iter);
        }

        value_descriptor::E_VALUE_TYPE cast_type = get_cast(left_iter);
        if (cast_type != value_descriptor::undefinedType) {
            value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
            code_emit_cast(left_value_type, cast_type);
        }

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

    void codegen::codegen_cmp_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::cmp_op_ID);

        //true_jmps_.push(jumps_t());
        //false_jmps_.push(jumps_t());

        iter_t left_iter = iter->children.begin();
        iter_t right_iter = left_iter + 1;

        if (left_iter->value.id() == freefoil_grammar::cmp_op_ID) {
            codegen_cmp_op(left_iter);
        } else {
            codegen_expr(left_iter);
        }

        value_descriptor::E_VALUE_TYPE cast_type = get_cast(left_iter);
        if (cast_type != value_descriptor::undefinedType) {
            value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
            code_emit_cast(left_value_type, cast_type);
        }

        codegen_expr(right_iter);

        cast_type = get_cast(right_iter);
        if (cast_type != value_descriptor::undefinedType) {
            value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();
            code_emit_cast(right_value_type, cast_type);
        }

        const std::string cmp_operation_as_str(parse_str(iter));
        if (cmp_operation_as_str == "==") {
            code_emit_branch(OPCODE_ifeq);
        } else if (cmp_operation_as_str == "!=") {
            code_emit_branch(OPCODE_ifneq);
        } else if (cmp_operation_as_str == "<=") {
            code_emit_branch(OPCODE_ifleq);
        } else if (cmp_operation_as_str == ">=") {
            code_emit_branch(OPCODE_ifgeq);
        } else if (cmp_operation_as_str == "<") {
            code_emit_branch(OPCODE_ifless);
        } else {
            assert(cmp_operation_as_str == ">");
            code_emit_branch(OPCODE_ifgreater);
        }

        //true_jmps_.top().push_back(code_chunks_.back().back());
        codechunk_t *true_jmp_to_be_backpatched = codechunks_.back().back();
        code_emit(OPCODE_push_false);
        code_emit_branch(OPCODE_jmp);
        //false_jmps_.top().push_back(code_chunks_.back().back());
        codechunk_t *jmp_to_be_backpatched = codechunks_.back().back();

        //set_jumps_dsts(true_jmps_.top(), code_chunks_.back().back());
        set_jmp_dst(true_jmp_to_be_backpatched, codechunks_.back().back());
        code_emit(OPCODE_push_true);
        //set_jumps_dsts(false_jmps_.top(), code_chunks_.back().back());
        set_jmp_dst(jmp_to_be_backpatched, codechunks_.back().back());

        //true_jmps_.pop();
        //false_jmps_.pop();

    }

    void codegen::code_emit(Runtime::BYTE opcode) {

        codechunk_shared_ptr_t pnew_codechunk(new codechunk_t);
        pnew_codechunk->bytecode_ = opcode;
        pnew_codechunk->is_plug_ = false;

        codechunks_pool_.push_back(pnew_codechunk);
        codechunks_.back().push_back(pnew_codechunk.get());
    }

    void codegen::code_emit(Runtime::BYTE opcode, Runtime::BYTE index) {

        code_emit(opcode);
        code_emit(index);
    }

    void codegen::code_emit_plug() {

        codechunk_shared_ptr_t pnew_codechunk(new codechunk_t);
        pnew_codechunk->is_plug_ = true;

        codechunks_pool_.push_back(pnew_codechunk);
        codechunks_.back().push_back(pnew_codechunk.get());
    }

    void codegen::code_emit_branch(Runtime::BYTE opcode) {

        code_emit(opcode);
        code_emit_plug(); //to be backpatched
    }

    void codegen::set_jmp_dst(codechunk_t *codechunk, const codechunk_t *dst_codechunk) {

        assert(codechunk->is_plug_ == true);
        codechunk->jump_dst_ = dst_codechunk;
    }

    void codegen::set_jumps_dsts(vector<codechunk_t *> &jmps_table, const codechunk_t *dst_codechunk) {

        for (vector<codechunk_t *>::iterator cur_iter = jmps_table.begin(), iter_end = jmps_table.end(); cur_iter != iter_end; ++cur_iter) {
            codechunk_t *codechunk = *cur_iter;
            assert(codechunk->is_plug_ == true);
            codechunk->jump_dst_ = dst_codechunk;
        }
    }

    void codegen::code_emit_cast(value_descriptor::E_VALUE_TYPE src_type, value_descriptor::E_VALUE_TYPE cast_type) {

        //possible implicit casts:
        /*
        str <-- int
        str <-- bool
        int <-- float
        int <-- bool
        float <-- bool
        */

        assert(src_type != cast_type and src_type != value_descriptor::stringType);
        if (src_type == value_descriptor::boolType) {
            assert(cast_type == value_descriptor::stringType or cast_type == value_descriptor::intType or cast_type == value_descriptor::floatType);
            if (cast_type == value_descriptor::stringType) {
                code_emit(OPCODE_b2str);
            } else if (cast_type == value_descriptor::intType) {
                //do nothing
            } else if (cast_type == value_descriptor::floatType) {
                code_emit(OPCODE_b2f);
            } else {
                assert(false);
            }
        } else if (src_type == value_descriptor::stringType) {
            assert(false);
        } else if (src_type == value_descriptor::floatType) {
            assert(cast_type == value_descriptor::intType);
            if (cast_type == value_descriptor::intType) {
                code_emit(OPCODE_f2i);
            } else {
                assert(false);
            }
        } else {
            assert(src_type == value_descriptor::intType);
            assert(cast_type == value_descriptor::stringType or cast_type == value_descriptor::floatType);
            if (cast_type == value_descriptor::floatType) {
                code_emit(OPCODE_i2f);
            } else {
                code_emit(OPCODE_i2str);
            }
        }
    }
}
