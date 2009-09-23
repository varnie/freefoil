#include "tree_analyzer.h"
#include "freefoil_grammar.h"
#include "AST_defs.h"
#include "exceptions.h"
#include "opcodes.h"

#include <iostream>
#include <list>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/bind/apply.hpp>
#include <boost/lexical_cast.hpp>

namespace Freefoil {

    using namespace Private;

    static value_descriptor::E_VALUE_TYPE get_greater_type(value_descriptor::E_VALUE_TYPE value_type1, value_descriptor::E_VALUE_TYPE value_type2);
    static value_descriptor::E_VALUE_TYPE get_greatest_common_type(value_descriptor::E_VALUE_TYPE value_type1, value_descriptor::E_VALUE_TYPE value_type2);
    static bool is_assignable(value_descriptor::E_VALUE_TYPE left_value_type, value_descriptor::E_VALUE_TYPE right_value_type);

    static std::string parse_str(const iter_t &iter);

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

    int tree_analyzer::find_assignable_function(const std::string &call_name, const std::vector<value_descriptor::E_VALUE_TYPE> &invoke_args, const function_shared_ptr_list_t &funcs) const{

        const std::size_t invoke_args_count = invoke_args.size();

        function_shared_ptr_list_t candidates_funcs;
        std::remove_copy_if(funcs.begin(),
                            funcs.end(),
                            std::back_inserter(candidates_funcs),
                            boost::bind(&function_descriptor::get_name, _1) != call_name or
                            boost::bind(&function_descriptor::get_param_descriptors_count, _1) != invoke_args_count
                           );

        std::vector<std::size_t> good_candidates_indexes;

        for (std::size_t i = 0, count = candidates_funcs.size(); i < count; ++i) {

            const function_shared_ptr_t tested_func = candidates_funcs[i];

            assert(invoke_args_count == tested_func->get_param_descriptors_count());
            const param_descriptors_shared_ptr_list_t params_list = tested_func->get_param_descriptors();
            for (std::size_t j = 0; j < invoke_args_count; ++j) {
                if (!is_assignable(params_list[j]->get_value_type(), invoke_args[j])) {
                    continue;
                }
            }

            good_candidates_indexes.push_back(i);
        }

        if (good_candidates_indexes.size() != 1){
        //    print_error("ambiguous function " + call_name + " call");
        //    ++errors_count_;
            return -1; //mark error
        }else{
            return good_candidates_indexes.front();
        }
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

    bool tree_analyzer::parse(const iter_t & tree_top) {

        std::cout << "analyze begin" << std::endl;

        errors_count_ = 0;
        funcs_list_.clear();

        const parser_id id = tree_top->value.id();
        assert(id == freefoil_grammar::script_ID || id == freefoil_grammar::func_decl_ID || id == freefoil_grammar::func_impl_ID);
        switch (id.to_long()) {
        case freefoil_grammar::script_ID:
            parse_script(tree_top);
            break;
        case freefoil_grammar::func_decl_ID:
            parse_func_decl(tree_top);
            break;
        case freefoil_grammar::func_impl_ID:
            parse_func_impl(tree_top);
            break;
        default:
            break;
        }

        //now we have all user-defined function heads parsed, as well as each user-defined function must have an iterator to its body tree structure (not parsed yet)
        //and we must have parsed "main" entry point function (with stored iterator on its body tree structure in it)
        //check this firstly:
        const function_shared_ptr_list_t::const_iterator invalid_function_iter
        = std::find_if(
              funcs_list_.begin(),
              funcs_list_.end(),
              boost::bind(&function_has_no_body_functor, _1));

        if (invalid_function_iter != funcs_list_.end()) {
            print_error("function " + (*invalid_function_iter)->get_name() + " is not implemented");
            ++errors_count_;
        }

        //entry point ("main" function) must be declared
        if (std::find_if(
                    funcs_list_.begin(),
                    funcs_list_.end(),
                    boost::bind(&entry_point_functor, _1)) == funcs_list_.end()) {
            print_error("entry point not declared");
            ++errors_count_;
        }

        //now we have all function declarations valid and we are sure we have "main" entry point function
        //it is a time for parsing function's bodies and generate intermediate code
        for (function_shared_ptr_list_t::const_iterator cur_iter = funcs_list_.begin(), iter_end = funcs_list_.end(); cur_iter != iter_end; ++cur_iter) {
            curr_parsing_function_ = *cur_iter;
            if (curr_parsing_function_->has_body()){
                parse_func_body(curr_parsing_function_->get_body());
                curr_parsing_function_->print_bytecode_stream();    //
            }
        }

        std::cout << "errors: " << errors_count_ << std::endl;
        std::cout << "analyze end" << std::endl;

        return errors_count_ == 0;
    }

    void tree_analyzer::setup_core_funcs() {

        //TODO: populate core_funcs_list_ with core functions
        param_descriptors_shared_ptr_list_t param_descriptors;
        param_descriptors.push_back(param_descriptor_shared_ptr_t(new param_descriptor(value_descriptor::intType, false, "i")));
        core_funcs_list_.push_back(function_shared_ptr_t (new function_descriptor("foo", value_descriptor::voidType, param_descriptors)));
    }

    tree_analyzer::tree_analyzer() :errors_count_(0), symbols_handler_(NULL), curr_parsing_function_(), stack_offset_(0) {
        setup_core_funcs();
    }

    void tree_analyzer::parse_script(const iter_t &iter) {

        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            if (cur_iter->value.id() == freefoil_grammar::func_decl_ID) {
                parse_func_decl(cur_iter);
            } else {
                assert(cur_iter->value.id() == freefoil_grammar::func_impl_ID);
                parse_func_impl(cur_iter);
            }
        }
    }

    void tree_analyzer::parse_func_decl(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_decl_ID);
        assert( (iter->children.begin() + 1)->value.id() == freefoil_grammar::stmt_end_ID);

        const function_shared_ptr_t parsed_func = parse_func_head(iter->children.begin());

        //is it a redeclaration of function?
        if (std::find_if(
                    funcs_list_.begin(),
                    funcs_list_.end(),
                    boost::bind(&function_heads_equal_functor, _1, parsed_func)) != funcs_list_.end()) {
            print_error(iter->children.begin(), "function " + parsed_func->get_name() + " already declared");
            ++errors_count_;
        } else {
            funcs_list_.push_back(parsed_func);
        }
    }

    void tree_analyzer::parse_func_impl(const iter_t &iter) {

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

            bool ok = true;
            //if that old function already has implementation, spit an error
            const function_shared_ptr_t old_func = *old_func_iter;
            if (old_func->has_body()) {
                print_error("function " + old_func->get_name() + " already implemented");
                ++errors_count_;
                ok = false;
            }
            //if that old function differs from our new one only by some "refs", spit an error
            assert(parsed_func->get_param_descriptors().size() == old_func->get_param_descriptors().size());
            if (!std::equal(
                        old_func->get_param_descriptors().begin(), old_func->get_param_descriptors().end(),
                        parsed_func->get_param_descriptors().begin(),
                        &param_descriptors_refs_equal_functor)) {
                print_error("previous decl differs from this one only by ref(s)");
                ++errors_count_;
                ok = false;
            }
            if (ok) {
                old_func->set_body(iter->children.begin()+1);
            }
        } else {
            //it is completely new function
            parsed_func->set_body(iter->children.begin()+1);
            funcs_list_.push_back(parsed_func);
        }
    }

    param_descriptor_shared_ptr_t tree_analyzer::parse_func_param_descriptor(const iter_t &iter) {

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

    param_descriptors_shared_ptr_list_t tree_analyzer::parse_func_param_descriptors_list(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::params_list_ID);

        stack_offset_ = 0;
        param_descriptors_shared_ptr_list_t param_descriptors_list;
        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            param_descriptors_list.push_back(parse_func_param_descriptor(cur_iter));
        }
        return param_descriptors_list;
    }

    function_shared_ptr_t tree_analyzer::parse_func_head(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_head_ID);
        assert(iter->children.size() == 3);
        assert((iter->children.begin())->value.id() == freefoil_grammar::func_type_ID);

        value_descriptor::E_VALUE_TYPE func_type;

        const std::string func_type_as_str(parse_str(iter->children.begin()));
        //TODO: add other function types checking
        if (func_type_as_str == "void") {
            func_type = value_descriptor::voidType;
        } else if (func_type_as_str == "string") {
            func_type = value_descriptor::stringType;
        } else if (func_type_as_str == "int") {
            func_type = value_descriptor::intType;
        } else if (func_type_as_str == "float") {
            func_type = value_descriptor::floatType;
        } else {
            assert(func_type_as_str == "bool");
            func_type = value_descriptor::boolType;
        }

        const function_shared_ptr_t parsed_func = function_shared_ptr_t(new function_descriptor(parse_str(iter->children.begin()+1), func_type, parse_func_param_descriptors_list(iter->children.begin()+2)));

        if (std::find_if(
                    core_funcs_list_.begin(),
                    core_funcs_list_.end(),
                    boost::bind(&function_heads_equal_functor, _1, parsed_func)) != core_funcs_list_.end()) {
            print_error(iter->children.begin(), "unable to override core function " + parsed_func->get_name());
            ++errors_count_;
        }

        return parsed_func;
    }

    void tree_analyzer::parse_var_declare_stmt_list(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::var_declare_stmt_list_ID);

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

            //parse var declare tails
            assert(cur_iter->value.id() == freefoil_grammar::var_declare_tail_ID);
            if (cur_iter->children.begin()->value.id() == freefoil_grammar::assign_op_ID) {
                assert(cur_iter->children.begin()->children.begin()->value.id() == freefoil_grammar::ident_ID);
                const std::string var_name(parse_str(cur_iter->children.begin()->children.begin()));

                if (!symbols_handler_->insert(var_name, value_descriptor(var_type, stack_offset_))) {
                    print_error(cur_iter->children.begin()->children.begin(), "redeclaration of variable " + var_name);
                    ++errors_count_;
                }
                create_attributes(cur_iter->children.begin()->children.begin(), var_type);

                if (cur_iter->children.begin()->children.begin() + 1 != cur_iter->children.begin()->children.end()) {
                    //it is an assign expr
                    parse_bool_expr(cur_iter->children.begin()->children.begin() + 1);
                    value_descriptor::E_VALUE_TYPE expr_val_type = (cur_iter->children.begin()->children.begin() + 1)->value.value().get_value_type();
                    if (is_assignable(var_type, expr_val_type)) {
                        if (var_type != expr_val_type) {
                            //make implicit cast explicit
                            create_cast(cur_iter->children.begin()->children.begin() + 1, var_type);
                        }
                        create_attributes(cur_iter->children.begin(), var_type);
                    } else {
                        print_error(cur_iter->children.begin(), "cannot assign " + type_to_string(expr_val_type) + " to " + type_to_string(var_type));
                        ++errors_count_;
                    }
                }
                ++stack_offset_;
            } else {
                assert(cur_iter->children.begin()->value.id() == freefoil_grammar::ident_ID);
                const std::string var_name(parse_str(cur_iter->children.begin()));
                symbols_handler_->insert(var_name, value_descriptor(var_type, stack_offset_));
                create_attributes(cur_iter->children.begin(), var_type);
                ++stack_offset_;
            }
        }
    }

    void tree_analyzer::parse_stmt(const iter_t &iter) {

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
            parse_var_declare_stmt_list(iter);
            break;
        }

        case freefoil_grammar::stmt_end_ID:
            break;

            //TODO: check for other stmts
        default:
            break;
        }
    }

    void tree_analyzer::parse_or_xor_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::or_xor_op_ID);

        if (iter->children.begin()->value.id() == freefoil_grammar::or_xor_op_ID) {
            parse_or_xor_op(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::bool_term_ID);
            assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::bool_term_ID);

            iter_t left_iter = iter->children.begin();
            iter_t right_iter = left_iter + 1;

            parse_bool_term(left_iter);
            parse_bool_term(right_iter);

            value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
            value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();

            if (left_value_type != value_descriptor::boolType or right_value_type != value_descriptor::boolType) {
                print_error(iter, "bool types expected for \"" + parse_str(iter) + "\" operator");
                ++errors_count_;
                create_attributes(iter, value_descriptor::undefinedType);
            } else {
                create_attributes(iter, value_descriptor::boolType);
            }
        }
    }

    void tree_analyzer::parse_and_op(const iter_t &iter) {

        assert(parse_str(iter) == "and");

        if (parse_str(iter->children.begin()) == "and") {
            parse_and_op(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::bool_factor_ID);
            assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::bool_factor_ID);

            iter_t left_iter = iter->children.begin();
            iter_t right_iter = left_iter + 1;

            parse_bool_factor(left_iter);
            parse_bool_factor(right_iter);

            value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
            value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();

            if (left_value_type != value_descriptor::boolType or right_value_type != value_descriptor::boolType) {
                print_error(iter, "bool types expected for \"and\" operator");
                ++errors_count_;
                create_attributes(iter, value_descriptor::undefinedType);
            } else {
                create_attributes(iter, value_descriptor::boolType);
            }
        }
    }

    void tree_analyzer::parse_plus_minus_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::plus_minus_op_ID);

        if (iter->children.begin()->value.id() == freefoil_grammar::plus_minus_op_ID) {
            parse_plus_minus_op(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::term_ID);
            assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::term_ID);

            iter_t left_iter = iter->children.begin();
            iter_t right_iter = left_iter + 1;

            parse_term(left_iter);
            parse_term(right_iter);

            value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
            value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();

            create_attributes(iter, get_greatest_common_type(left_value_type,
                              right_value_type));

            value_descriptor::E_VALUE_TYPE iter_value_type = iter->value.value().get_value_type();

            if (iter_value_type == value_descriptor::undefinedType) {
                if (parse_str(iter) == "+") {
                    print_error(iter, "cannot add expressions of types " + type_to_string(left_value_type) + " and " + type_to_string(right_value_type));
                } else {
                    assert(parse_str(iter) == "-");
                    print_error(iter, "cannot subtract expressions of types " + type_to_string(left_value_type) + " and " + type_to_string(right_value_type));
                }
                ++errors_count_;
            } else {
                if (iter_value_type != left_value_type) {
                    create_cast(left_iter, iter_value_type);
                }
                if (iter_value_type != right_value_type) {
                    create_cast(right_iter, iter_value_type);
                }
            }
        }
    }

    void tree_analyzer::parse_mult_divide_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::mult_divide_op_ID);

        if (iter->children.begin()->value.id() == freefoil_grammar::mult_divide_op_ID) {
            parse_mult_divide_op(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::factor_ID);
            assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::factor_ID);

            iter_t left_iter = iter->children.begin();
            iter_t right_iter = left_iter + 1;

            parse_factor(left_iter);
            parse_factor(right_iter);

            value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
            value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();

            create_attributes(iter, get_greatest_common_type(left_value_type,
                              right_value_type));

            value_descriptor::E_VALUE_TYPE iter_value_type = iter->value.value().get_value_type();

            if (iter_value_type == value_descriptor::undefinedType) {
                if (parse_str(iter) == "*") {
                    print_error(iter, "cannot multiplicate expressions of types " + type_to_string(left_value_type) + " and " + type_to_string(right_value_type));
                } else {
                    assert(parse_str(iter) == "/");
                    print_error(iter, "cannot divide expressions of types " + type_to_string(left_value_type) + " and " + type_to_string(right_value_type));
                }
                ++errors_count_;
            } else {
                if (iter_value_type != left_value_type) {
                    create_cast(left_iter, iter_value_type);
                }
                if (iter_value_type != right_value_type) {
                    create_cast(right_iter, iter_value_type);
                }
            }
        }
    }


    void tree_analyzer::parse_cmp_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::cmp_op_ID);

        if (iter->children.begin()->value.id() == freefoil_grammar::cmp_op_ID) {
            parse_cmp_op(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::expr_ID);
            assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::expr_ID);

            iter_t left_iter = iter->children.begin();
            iter_t right_iter = left_iter + 1;

            parse_expr(left_iter);
            parse_expr(right_iter);

            value_descriptor::E_VALUE_TYPE left_value_type = left_iter->value.value().get_value_type();
            value_descriptor::E_VALUE_TYPE right_value_type = right_iter->value.value().get_value_type();

            create_attributes(iter, get_greatest_common_type(left_value_type,
                              right_value_type));

            value_descriptor::E_VALUE_TYPE iter_value_type = iter->value.value().get_value_type();

            if (iter_value_type == value_descriptor::undefinedType) {
                print_error(iter, "cannot compare expressions of types " + type_to_string(left_value_type) + " and " + type_to_string(right_value_type));
                ++errors_count_;
            } else {
                if (iter_value_type != left_value_type) {
                    create_cast(left_iter, iter_value_type);
                }
                if (iter_value_type != right_value_type) {
                    create_cast(right_iter, iter_value_type);
                }
                create_attributes(iter, value_descriptor::boolType);
            }
        }
    }

    void tree_analyzer::parse_bool_expr(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_expr_ID);

        const parser_id id = iter->children.begin()->value.id();
        if (id == freefoil_grammar::bool_term_ID) {
            parse_bool_term(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        } else {
            assert(id == freefoil_grammar::or_xor_op_ID);
            parse_or_xor_op(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        }
    }

    void tree_analyzer::parse_expr(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::expr_ID);

        const bool has_unary_plus_minus_op = iter->children.begin()->value.id() == freefoil_grammar::unary_plus_minus_op_ID;
        if (has_unary_plus_minus_op) {
            const parser_id id = (iter->children.begin() + 1)->value.id();
            if (id == freefoil_grammar::term_ID) {
                parse_term(iter->children.begin() + 1);
                create_attributes(iter, (iter->children.begin() + 1)->value.value().get_value_type());
            } else {
                assert(id == freefoil_grammar::plus_minus_op_ID);
                parse_plus_minus_op(iter->children.begin() + 1);
                create_attributes(iter, (iter->children.begin() + 1)->value.value().get_value_type());
            }
        } else {
            const parser_id id = iter->children.begin()->value.id();
            if (id == freefoil_grammar::term_ID) {
                parse_term(iter->children.begin());
                create_attributes(iter, iter->children.begin()->value.value().get_value_type());
            } else {
                assert(id == freefoil_grammar::plus_minus_op_ID);
                parse_plus_minus_op(iter->children.begin());
                create_attributes(iter, iter->children.begin()->value.value().get_value_type());
            }
        }
    }

    void tree_analyzer::parse_term(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::term_ID);

        if (iter->children.begin()->value.id() == freefoil_grammar::factor_ID) {
            parse_factor(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::mult_divide_op_ID);
            parse_mult_divide_op(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        }
    }

    void tree_analyzer::parse_ident(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::ident_ID);

        const string name(parse_str(iter));
        int stack_offset;
        value_descriptor::E_VALUE_TYPE value_type = value_descriptor::undefinedType;
        const value_descriptor *the_value_descriptor = symbols_handler_->lookup(name);
        if (the_value_descriptor != NULL) {
            value_type = the_value_descriptor->get_value_type();
            stack_offset = the_value_descriptor->get_stack_offset();
        } else {
            const param_descriptors_shared_ptr_list_t::const_iterator suitable_param_descriptor_iter
            = std::find_if(
                  curr_parsing_function_->get_param_descriptors().begin(),
                  curr_parsing_function_->get_param_descriptors().end(),
                  boost::bind(&param_descriptor_has_name_functor, _1, name));
            if (suitable_param_descriptor_iter != curr_parsing_function_->get_param_descriptors().end()) {
                value_type = (*suitable_param_descriptor_iter)->get_value_type();
                stack_offset = (*suitable_param_descriptor_iter)->get_stack_offset();
            } else {
                //error. such variable is unknown
                print_error(iter, "unknown ident " + name);
                ++errors_count_;
            }
        }

        create_attributes(iter, value_type, stack_offset);
    }

    void tree_analyzer::parse_factor(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::factor_ID);

        switch (iter->children.begin()->value.id().to_long()) {
        case freefoil_grammar::func_call_ID:
            parse_func_call(iter->children.begin());
            break;
        case freefoil_grammar::ident_ID:
            parse_ident(iter->children.begin());
            break;
        case freefoil_grammar::number_ID:
            parse_number(iter->children.begin());
            break;
        case freefoil_grammar::quoted_string_ID:
            parse_quoted_string(iter->children.begin());
            break;
        case freefoil_grammar::bool_constant_ID:
            parse_bool_constant(iter->children.begin());
            break;
        case freefoil_grammar::bool_expr_ID:
            parse_bool_expr(iter->children.begin());
        }

        create_attributes(iter, iter->children.begin()->value.value().get_value_type());
    }

    void tree_analyzer::parse_func_call(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_call_ID);

        const std::string func_name(parse_str(iter->children.begin()));
        assert((iter->children.begin() + 1)->value.id() == freefoil_grammar::invoke_args_list_ID);
        std::vector<value_descriptor::E_VALUE_TYPE> invoked_value_types;
        for (iter_t cur_iter = (iter->children.begin() + 1)->children.begin(), iter_end = (iter->children.begin() + 1)->children.end(); cur_iter != iter_end; ++cur_iter) {

            assert(cur_iter->value.id() == freefoil_grammar::bool_expr_ID);
            parse_bool_expr(cur_iter);
            invoked_value_types.push_back(cur_iter->value.value().get_value_type());
        }

        if (int result = find_assignable_function(func_name, invoked_value_types, funcs_list_) != -1){
            create_attributes(iter, funcs_list_[result]->get_type(), result);
        }else{
            if ((result = find_assignable_function(func_name, invoked_value_types, core_funcs_list_)) != -1){
                create_attributes(iter, core_funcs_list_[result]->get_type(), result);
            }else{
                print_error(iter, "unable to call function " + func_name);
                ++errors_count_;
                create_attributes(iter, value_descriptor::undefinedType);
            }
        }
    }

    void tree_analyzer::parse_bool_constant(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_constant_ID);

        create_attributes(iter, value_descriptor::boolType);
    }

    void tree_analyzer::parse_bool_term(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_term_ID);

        const parser_id id = iter->children.begin()->value.id();
        if (id == freefoil_grammar::bool_factor_ID) {
            parse_bool_factor(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        } else {
            assert(parse_str(iter->children.begin()) == "and");

            parse_and_op(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        }
    }

    void tree_analyzer::parse_bool_factor(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_factor_ID);

        const bool negate = (parse_str(iter->children.begin()) == "not");
        parse_bool_relation(negate ? iter->children.begin() + 1 : iter->children.begin());
        if (negate) {
            create_attributes(iter, (iter->children.begin() + 1)->value.value().get_value_type());
            if ((iter->children.begin() + 1)->value.value().get_value_type() != value_descriptor::boolType) {
                print_error(iter, "cannot perform \"not\" operator for not bool type " + type_to_string((iter->children.begin() + 1)->value.value().get_value_type()));
                ++errors_count_;
            }
        } else {
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        }
    }

    void tree_analyzer::parse_bool_relation(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_relation_ID);

        const parser_id id = iter->children.begin()->value.id();
        if (id == freefoil_grammar::expr_ID) {
            parse_expr(iter->children.begin());
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        } else {
            assert(id == freefoil_grammar::cmp_op_ID);

            parse_cmp_op(iter->children.begin());

            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        }
    }

    void tree_analyzer::parse_number(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::number_ID);

        const std::string number_as_str(parse_str(iter));
        if (number_as_str.find('.') != std::string::npos) {
            //it is float value
            const std::size_t index = curr_parsing_function_->add_float_constant(boost::lexical_cast<float>(number_as_str));
            create_attributes(iter, value_descriptor::floatType, (int) index);
        } else {
            //it is int value
            const std::size_t index = curr_parsing_function_->add_int_constant(boost::lexical_cast<int>(number_as_str));
            create_attributes(iter, value_descriptor::intType, (int) index);
        }
    }

    void tree_analyzer::parse_quoted_string(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::quoted_string_ID);
        const std::string quoted_string(parse_str(iter));
        const std::string str_value_without_quotes(quoted_string.begin() + 1, quoted_string.end() - 1);

        const std::size_t index = curr_parsing_function_->add_string_constant(str_value_without_quotes);
        create_attributes(iter, value_descriptor::stringType, (int) index);
    }

    void tree_analyzer::parse_func_body(const iter_t &iter) {

        stack_offset_ = curr_parsing_function_->get_param_descriptors_count();

        symbols_handler_.reset(new symbols_handler);
        symbols_handler_->scope_begin();

        assert(iter->value.id() == freefoil_grammar::func_body_ID);
        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            parse_stmt(cur_iter);
        }

        symbols_handler_->scope_end();
    }

    void tree_analyzer::print_error(const iter_t &iter, const std::string &msg) {
        std::cout << "line " << iter->value.begin().get_position().line << " ";
        std::cout << msg << std::endl;
    }

    void tree_analyzer::print_error(const std::string &msg) {
        std::cout << msg << std::endl;
    }

    void tree_analyzer::create_cast(const iter_t &iter, const value_descriptor::E_VALUE_TYPE cast_type) {
        node_attributes tmp(iter->value.value());
        tmp.set_cast(cast_type);
        iter->value.value(tmp);
    }

    void tree_analyzer::create_attributes(const iter_t &iter, const value_descriptor::E_VALUE_TYPE value_type) {
        node_attributes tmp(iter->value.value());
        tmp.set_value_type(value_type);
        iter->value.value(tmp);
    }

    void tree_analyzer::create_attributes(const iter_t &iter, const value_descriptor::E_VALUE_TYPE value_type, const int index) {
        node_attributes tmp(iter->value.value());
        tmp.set_value_type(value_type);
        tmp.set_index(index);
        iter->value.value(tmp);
    }

////////////////////////////////////////////////////////////////////////////////////////////////////
    static std::string parse_str(const iter_t &iter) {
        return std::string(iter->value.begin(), iter->value.end());
    }

    static value_descriptor::E_VALUE_TYPE get_greater_type(value_descriptor::E_VALUE_TYPE value_type1, value_descriptor::E_VALUE_TYPE value_type2) {
        /*
        if (value_type1 == value_descriptor::undefinedType or value_type2 == value_descriptor::undefinedType) {
            return value_descriptor::undefinedType;
        }*/
        if (value_type1 == value_type2) {
            return value_type1;
        }
        if (value_type1 == value_descriptor::stringType and
                (value_type2 == value_descriptor::intType or value_type2 == value_descriptor::boolType)) {
            return value_descriptor::stringType;
        }
        if (value_type1 == value_descriptor::intType and value_type2 == value_descriptor::floatType) {
            return value_descriptor::floatType;
        }
        if (value_type1 == value_descriptor::intType and value_type2 == value_descriptor::boolType) {
            return value_descriptor::intType;
        }

        return value_descriptor::undefinedType;
    }

    static value_descriptor::E_VALUE_TYPE get_greatest_common_type(value_descriptor::E_VALUE_TYPE value_type1, value_descriptor::E_VALUE_TYPE value_type2) {

        value_descriptor::E_VALUE_TYPE result_type;

        if ((result_type = get_greater_type(value_type1, value_type2)) != value_descriptor::undefinedType) {
            return result_type;
        }
        if ((result_type = get_greater_type(value_type2, value_type1)) != value_descriptor::undefinedType) {
            return result_type;
        }
        return value_descriptor::undefinedType;
    }

    static bool is_assignable(value_descriptor::E_VALUE_TYPE left_value_type, value_descriptor::E_VALUE_TYPE right_value_type) {
        /*
        if (left_value_type == value_descriptor::undefinedType or right_value_type == value_descriptor::undefinedType){
            return true;
        }*/
        return get_greater_type(left_value_type, right_value_type) != value_descriptor::undefinedType;
    }
}
