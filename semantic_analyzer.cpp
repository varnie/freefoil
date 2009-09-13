#include "semantic_analyzer.h.h"

#include "freefoil_grammar.h"
#include "AST_defs.h"
#include "exceptions.h"
#include "opcodes.h"

#include <iostream>
#include <list>
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>



namespace Freefoil {

    using namespace Private;

    bool param_descriptors_types_equal_functor(const param_descriptor_shared_ptr_t &param_descriptor, const param_descriptor_shared_ptr_t &the_param_descriptor) {
        return 	param_descriptor->get_value_type() == the_param_descriptor->get_value_type();
    }

    bool param_descriptors_refs_equal_functor(const param_descriptor_shared_ptr_t &param_descriptor, const param_descriptor_shared_ptr_t &the_param_descriptor) {
        return 	param_descriptor->is_ref() == the_param_descriptor->is_ref();
    }

    bool function_heads_equal_functor(const function_shared_ptr_t &func, const function_shared_ptr_t &the_func) {
        return 		func->get_type() == the_func->get_type()
                 &&  func->get_name() == the_func->get_name()
                 &&  func->get_param_descriptors().size() == the_func->get_param_descriptors().size()
                 &&  std::equal(
                     func->get_param_descriptors().begin(), func->get_param_descriptors().end(),
                     the_func->get_param_descriptors().begin(),
                     &param_descriptors_types_equal_functor);
    }

    bool function_has_no_body_functor(const function_shared_ptr_t &the_func) {
        return 	!the_func->has_body();
    }

    bool entry_point_functor(const function_shared_ptr_t &the_func) {
        return 	the_func->get_name() == "main";
    }

    bool param_descriptor_has_name_functor(const param_descriptor_shared_ptr_t &the_param_descriptor, const std::string &the_name) {
        return the_param_descriptor->get_name() == the_name;
    }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    semantic_analyzer::semantic_analyzer(): symbols_handler_(NULL), curr_parsing_function_(), stack_offset_(0) {
        setup_core_funcs();
    }

    void semantic_analyzer::analyze(const iter_t &iter) {

        std::cout << "semantic analyze begin" << std::endl;

        try {
            const parser_id id = iter->value.id();
            assert(id == freefoil_grammar::script_ID || id == freefoil_grammar::func_decl_ID || id == freefoil_grammar::func_impl_ID);
            switch (id.to_long()) {
            case freefoil_grammar::script_ID:
                analyze_script(iter);
                break;
            case freefoil_grammar::func_decl_ID:
                analyze_func_decl(iter);
                break;
            case freefoil_grammar::func_impl_ID:
                analyze_func_impl(iter);
                break;
            default:
                break;
            }

            //now we have all user-defined function heads parsed, as well as each user-defined function must has an iterator to its body tree structure (not parsed yet)
            //and we must have parsed "main" entry point function (with stored iterator on its body tree structure in it)
            //check this firstly:
            const function_shared_ptr_list_t::const_iterator invalid_function_iter
            = std::find_if(
                  funcs_list_.begin(),
                  funcs_list_.end(),
                  boost::bind(&function_has_no_body_functor, _1));

            if (invalid_function_iter != funcs_list_.end()) {
                throw freefoil_exception("function " + (*invalid_function_iter)->get_name() + " is not implemented");
            }

            //entry point ("main" function) must be declared
            if (std::find_if(
                        funcs_list_.begin(),
                        funcs_list_.end(),
                        boost::bind(&entry_point_functor, _1)) == funcs_list_.end()) {
                throw freefoil_exception("entry point not declared");
            }

            //now we have all function declarations valid and we are sure we have "main" entry point function
            //it is a time for parsing function's bodies and generate intermediate code
            for (function_shared_ptr_list_t::const_iterator cur_iter = funcs_list_.begin(), iter_end = funcs_list_.end(); cur_iter != iter_end; ++cur_iter) {
                curr_parsing_function_ = *cur_iter;
                parse_func_body(curr_parsing_function_->get_body());
//                curr_parsing_function_->print_bytecode_stream();
            }

            //TODO:
            //
        } catch (const freefoil_exception &e) {
            std::cout << "catched: " << e.what() << std::endl;
        }

        std::cout << "semantic analyze end" << std::endl;
    }

    void semantic_analyzer::setup_core_funcs() {

        //TODO: populate core_funcs_list_ with core functions
        param_descriptors_shared_ptr_list_t param_descriptors;
        param_descriptors.push_back(param_descriptor_shared_ptr_t(new param_descriptor(value_descriptor::intType, false, "i")));
        core_funcs_list_.push_back(function_shared_ptr_t (new function_descriptor("foo", function_descriptor::voidType, param_descriptors)));
    }

    void semantic_analyzer::analyze_script(const iter_t &iter) {

        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            if (cur_iter->value.id() == freefoil_grammar::func_decl_ID) {
                analyze_func_decl(cur_iter);
            } else {
                assert(cur_iter->value.id() == freefoil_grammar::func_impl_ID);
                analyze_func_impl(cur_iter);
            }
        }
    }

    void semantic_analyzer::parse_func_decl(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_decl_ID);

        stack_offset_ = 0;

        const function_shared_ptr_t parsed_func = parse_func_head(iter->children.begin());

        //is it a redeclaration of function?
        if (std::find_if(
                    funcs_list_.begin(),
                    funcs_list_.end(),
                    boost::bind(&function_heads_equal_functor, _1, parsed_func)) != funcs_list_.end()) {
            throw freefoil_exception("function " + parsed_func->get_name() + " already declared");
        }

        assert( (iter->children.begin() + 1)->value.id() == freefoil_grammar::stmt_end_ID);
        funcs_list_.push_back(parsed_func);
    }

    void semantic_analyzer::parse_func_impl(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_impl_ID);
        const function_shared_ptr_t parsed_func = parse_func_head(iter->children.begin());

        //is it an implementation of previously declared function?
        const function_shared_ptr_list_t::const_iterator old_func_iter
        =	std::find_if(
              funcs_list_.begin(),
              funcs_list_.end(),
              boost::bind(&function_heads_equal_functor, _1, parsed_func)
          );
        if (old_func_iter != funcs_list_.end()) {
            //found previous declaration

            //if that old function already has implementation, spit an error
            const function_shared_ptr_t old_func = *old_func_iter;
            if (old_func->has_body()) {
                throw freefoil_exception("function " + old_func->get_name() + " already implemented");
            }
            //if that old function differs from our new one only by some "refs", spit an error
            assert(parsed_func->get_param_descriptors().size() == old_func->get_param_descriptors().size());
            if (!std::equal(
                        old_func->get_param_descriptors().begin(), old_func->get_param_descriptors().end(),
                        parsed_func->get_param_descriptors().begin(),
                        &param_descriptors_refs_equal_functor)) {
                throw freefoil_exception("previous declaration differs from this one only by \"ref(s)\"");
            }
            old_func->set_body(iter->children.begin()+1);
        } else {
            //it is completely new function
            parsed_func->set_body(iter->children.begin()+1);
            std::cout <<  "func_body_iter type = " << (*(iter->children.begin()+1)).value.id().to_long() << std::endl;
            funcs_list_.push_back(parsed_func);
        }
    }

    param_descriptor_shared_ptr_t script::parse_func_param_descriptor(const iter_t &iter) {
        assert(iter->value.id() == freefoil_grammar::param_ID);

        value_descriptor::E_VALUE_TYPE val_type;
        bool is_ref = false;
        std::string val_name = "";

        const std::string val_type_as_str(parse_str(iter->children.begin()));
        //TODO: add other function types checking
        if (val_type_as_str == "int") {
            val_type = value_descriptor::intType;
        } else if (val_type_as_str == "float") {
            val_type = value_descriptor::floatType;
        } else if (val_type_as_str == "bool") {
            val_type = value_descriptor::boolType;
        } else {
            assert(val_type_as_str == "string");
            val_type = value_descriptor::stringType;
        }

        iter_t cur_iter = iter->children.begin() + 1;
        const iter_t iter_end = iter->children.end();
        if (cur_iter != iter_end) {
            if (cur_iter->value.id() == freefoil_grammar::ref_ID) {
                is_ref = true;
                ++cur_iter;
            }
            if (cur_iter != iter_end) {
                if (cur_iter->value.id() == freefoil_grammar::ident_ID) {
                    val_name = parse_str(cur_iter);
                    ++cur_iter;
                }
            }
        }
        assert(cur_iter == iter_end);
        return param_descriptor_shared_ptr_t(new param_descriptor(val_type, stack_offset_++, val_name, is_ref));
    }

    param_descriptors_shared_ptr_list_t semantic_analyzer::parse_func_param_descriptors_list(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::params_list_ID);
        param_descriptors_shared_ptr_list_t param_descriptors_list;
        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            param_descriptors_list.push_back(parse_func_param_descriptor(cur_iter));
        }
        return param_descriptors_list;
    }

    function_shared_ptr_t semantic_analyzer::parse_func_head(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_head_ID);
        assert(iter->children.size() == 3);
        assert((iter->children.begin())->value.id() == freefoil_grammar::func_type_ID);
        function_descriptor::E_FUNCTION_TYPE func_type;

        const std::string func_type_as_str(parse_str(iter->children.begin()));
        //TODO: add other function types checking
        if (func_type_as_str == "void") {
            func_type = function_descriptor::voidType;
        } else if (func_type_as_str == "string") {
            func_type = function_descriptor::stringType;
        } else if (func_type_as_str == "int") {
            func_type = function_descriptor::intType;
        } else if (func_type_as_str == "float") {
            func_type = function_descriptor::floatType;
        } else {
            assert(func_type_as_str == "bool");
            func_type = function_descriptor::boolType;
        }
        std::cout << "func_type: " << func_type;

        const function_shared_ptr_t parsed_func = function_shared_ptr_t(new function_descriptor(parse_str(iter->children.begin()+1), func_type, parse_func_param_descriptors_list(iter->children.begin()+2)));

        if (std::find_if(
                    core_funcs_list_.begin(),
                    core_funcs_list_.end(),
                    boost::bind(&function_heads_equal_functor, _1, parsed_func)) != core_funcs_list_.end()) {
            throw freefoil_exception("unable to override core function");
        }

        return parsed_func;
    }

    void semantic_analyzer::parse_stmt(const iter_t &iter) {

        switch (iter->value.id().to_long()) {
        case freefoil_grammar::block_ID: {
            const int stack_offset = stack_offset_;
            symbols_handler_->scope_begin();
            parse_stmt(iter->children.begin());
            symbols_handler_->scope_end();
            stack_offset_ = stack_offset;
            break;
        }
        case freefoil_grammar::var_declare_stmt_list_ID: {
            const std::string var_type_as_str(parse_str(iter->children.begin()));
            value_descriptor::E_VALUE_TYPE var_type;
            //TODO: add checking for other possible types
            if (var_type_as_str == "string") {
                var_type = value_descriptor::stringType;
            } else if (var_type_as_str == "int") {
                var_type = value_descriptor::intType;
            } else if (var_type_as_str == "float") {
                var_type = value_descriptor::floatType;
            } else {
                assert(var_type_as_str == "bool");
                var_type = value_descriptor::boolType;
            }

            for (iter_t cur_iter = iter->children.begin() + 1, iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {

                //parse var decl
                assert(cur_iter->value.id() == freefoil_grammar::var_declare_tail_ID);
                const std::string var_name(parse_str(cur_iter->children.begin()));

                symbols_handler_->insert(var_name, value_descriptor(var_type, stack_offset_));

                if (cur_iter->children.begin() + 1 != cur_iter->children.end()) {
                    //it is an assign expr
                    parse_bool_expr(cur_iter->children.begin() + 2);

                    curr_parsing_function_->add_instruction(instruction(Private::STORE_VAR));
                    curr_parsing_function_->add_instruction(instruction(Private::GET_VAR_INDEX, stack_offset_));
                }
                ++stack_offset_;
            }
            break;
        }

        case freefoil_grammar::stmt_end_ID:
            break;

            //TODO: check for other stmts
        default:
            break;
        }
    }

    void semantic_analyzer::parse_bool_expr(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_expr_ID);

        parse_bool_term(iter->children.begin());
        if (iter->children.begin() + 1 != iter->children.end()) {
            parse_or_tail(iter->children.begin() + 1);
        }
    }

    void semantic_analyzer::parse_or_tail(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::or_tail_ID);

        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ) {

            const std::string or_operation_as_str(parse_str(cur_iter++));
            parse_bool_term(cur_iter++);
            if (or_operation_as_str == "or") {
                curr_parsing_function_->add_instruction(instruction(Private::OR_OP));
            } else {
                assert(or_operation_as_str == "xor");
                curr_parsing_function_->add_instruction(instruction(Private::XOR_OP));
            }
        }
    }

    void semantic_analyzer::parse_and_tail(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::and_tail_ID);
        assert(parse_str(iter->children.begin()) == "and");

        iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end();
        while (cur_iter != iter_end && parse_str(cur_iter) == "and") {
            parse_bool_factor(++cur_iter);
            curr_parsing_function_->add_instruction(instruction(Private::AND_OP));
        }
    }

    void semantic_analyzer::parse_expr(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::expr_ID);

        const bool has_unary_plus_minus_op = iter->children.begin()->value.id() == freefoil_grammar::plus_minus_op_ID;
        iter_t cur_iter = has_unary_plus_minus_op ? iter->children.begin() + 1 : iter->children.begin();
        const iter_t iter_end = iter->children.end();
        parse_term(cur_iter++);

        while (cur_iter != iter_end) {
            assert(cur_iter->value.id() == freefoil_grammar::plus_minus_op_ID);
            if (parse_str(cur_iter) == "+") {
                parse_term(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::PLUS_OP));
            } else {
                assert(parse_str(cur_iter) == "-");
                parse_term(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::MINUS_OP));
            }
            ++cur_iter;
        }

        if (has_unary_plus_minus_op) {
            if (parse_str(iter->children.begin()) == "-") {
                curr_parsing_function_->add_instruction(instruction(Private::NEGATE_OP));
            }
        }
    }

    void semantic_analyzer::parse_term(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::term_ID);

        iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end();
        parse_factor(cur_iter++);

        while (cur_iter != iter_end) {
            assert(cur_iter->value.id() == freefoil_grammar::mult_divide_op_ID);
            if (parse_str(cur_iter) == "*") {
                parse_factor(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::MULT_OP));
            } else {
                assert(parse_str(cur_iter) == "/");
                parse_factor(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::DIVIDE_OP));
            }
            ++cur_iter;
        }
    }

    void semantic_analyzer::parse_factor(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::factor_ID);

        switch (iter->children.begin()->value.id().to_long()) {
        case freefoil_grammar::ident_ID: {
            const string name(parse_str(iter->children.begin()));
            int stack_offset;
            const value_descriptor *the_value_descriptor = symbols_handler_->lookup(name);
            if (the_value_descriptor != NULL) {
                stack_offset = the_value_descriptor->get_stack_offset();
            } else {
                const param_descriptors_shared_ptr_list_t::const_iterator suitable_param_descriptor_iter
                = std::find_if(
                      curr_parsing_function_->get_param_descriptors().begin(),
                      curr_parsing_function_->get_param_descriptors().end(),
                      boost::bind(&param_descriptor_has_name_functor, _1, name));
                if (suitable_param_descriptor_iter != curr_parsing_function_->get_param_descriptors().end()) {
                    stack_offset = (*suitable_param_descriptor_iter)->get_stack_offset();
                } else {
                    //error. such variable not presented
                    throw freefoil_exception("unknown ident: " + name);
                }
            }

            curr_parsing_function_->add_instruction(instruction(Private::LOAD_VAR));
            curr_parsing_function_->add_instruction(instruction(Private::GET_VAR_INDEX, stack_offset));
            break;
        }

        case freefoil_grammar::number_ID: {
            parse_number(iter->children.begin());
            break;
        }

        case freefoil_grammar::quoted_string_ID: {
            parse_quoted_string(iter->children.begin());
            break;
        }

        case freefoil_grammar::bool_expr_ID:
            parse_bool_expr(iter->children.begin());
            break;

        case freefoil_grammar::func_call_ID:
            parse_func_call(iter->children.begin());
            break;

        case freefoil_grammar::boolean_constant_ID:
            parse_boolean_constant(iter->children.begin());
            break;

        default:
            //can't occur
            break;
        }
    }

    void semantic_analyzer::parse_func_call(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_call_ID);

        const std::string func_name(parse_str(iter->children.begin()));
        for (iter_t cur_iter = (iter->children.begin() + 2)->children.begin(), iter_end = (iter->children.begin() + 2)->children.end(); cur_iter != iter_end; ++cur_iter) {
            //TODO:
        }
        //TODO:
    }

    void semantic_analyzer::parse_boolean_constant(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::boolean_constant_ID);

        const std::string boolean_constant_as_str(parse_str(iter));
        if (boolean_constant_as_str == "true") {
            curr_parsing_function_->add_instruction(instruction(Private::PUSH_TRUE));
        } else {
            assert(boolean_constant_as_str == "false");
            curr_parsing_function_->add_instruction(instruction(Private::PUSH_FALSE));
        }
    }

    void semantic_analyzer::parse_bool_term(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_term_ID);

        parse_bool_factor(iter->children.begin());
        if (iter->children.begin() + 1 != iter->children.end()) {
            parse_and_tail(iter->children.begin() + 1);
        }
    }

    void semantic_analyzer::parse_bool_factor(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_factor_ID);

        const bool negate = (parse_str(iter->children.begin()) == "not");
        parse_bool_relation(negate ? iter->children.begin() + 1 : iter->children.begin());
        if (negate) {
            curr_parsing_function_->add_instruction(instruction(Private::NOT_OP));
        }
    }

    void semantic_analyzer::parse_bool_relation(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_relation_ID);

        iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end();
        parse_expr(cur_iter++);

        while (cur_iter != iter_end) {
            assert(cur_iter->value.id() == freefoil_grammar::relation_tail_ID);
            const std::string relation_op_as_str(parse_str(cur_iter));

            if (relation_op_as_str == "==") {
                parse_expr(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::EQUAL_OP));
            } else if (relation_op_as_str == "!=") {
                parse_expr(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::NOT_EQUAL_OP));
            } else if (relation_op_as_str == "<=") {
                parse_expr(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::LESS_OR_EQUAL_OP));
            } else if (relation_op_as_str == ">=") {
                parse_expr(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::GREATER_OR_EQUAL_OP));
            } else if (relation_op_as_str == "<") {
                parse_expr(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::LESS_OP));
            } else {
                assert(relation_op_as_str == ">");
                parse_expr(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::GREATER_OP));
            }
        }
    }

    void semantic_analyzer::parse_number(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::number_ID);

        const std::string number_as_str(parse_str(iter));
        if (number_as_str.find('.') != std::string::npos) {
            //it is float value
            const std::size_t index = curr_parsing_function_->add_float_constant(boost::lexical_cast<float>(number_as_str));
            curr_parsing_function_->add_instruction(instruction(Private::GET_FLOAT_CONST));
            curr_parsing_function_->add_instruction(instruction(Private::GET_INDEX_OF_CONST, (int)index));
        } else {
            //it is int value
            const std::size_t index = curr_parsing_function_->add_int_constant(boost::lexical_cast<int>(number_as_str));
            curr_parsing_function_->add_instruction(instruction(Private::GET_INT_CONST));
            curr_parsing_function_->add_instruction(instruction(Private::GET_INDEX_OF_CONST, (int)index));
        }
    }

    void semantic_analyzer::parse_quoted_string(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::quoted_string_ID);
        const std::string quoted_string(parse_str(iter));
        const std::string str_value_without_quotes(quoted_string.begin() + 1, quoted_string.end() - 1);

        const std::size_t index = curr_parsing_function_->add_string_constant(str_value_without_quotes);
        curr_parsing_function_->add_instruction(instruction(Private::GET_STRING_CONST));
        curr_parsing_function_->add_instruction(instruction(Private::GET_INDEX_OF_CONST, (int)index));
    }

    void semantic_analyzer::parse_func_body(const iter_t &iter) {

        stack_offset_ = curr_parsing_function_->get_param_descriptors_count();

        symbols_handler_.reset(new symbols_handler);
        symbols_handler_->scope_begin();

        assert(iter->value.id() == freefoil_grammar::func_body_ID);
        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            parse_stmt(cur_iter);
        }

        symbols_handler_->scope_end();
    }

    std::string semantic_analyzer::parse_str(const iter_t &iter) {
        return std::string(iter->value.begin(), iter->value.end());
    }

    ///////////////////////////////////////////////////////////////////////////////////////
    static tree_parse_info_t build_AST(const iterator_t &iter_begin, const iterator_t &iter_end) {
        return ast_parse<factory_t>(iter_begin, iter_end, freefoil_grammar(), space_p);
    }
}
