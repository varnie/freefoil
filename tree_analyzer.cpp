#include "tree_analyzer.h"
#include "freefoil_grammar.h"
#include "AST_defs.h"
#include "defs.h"
#include "runtime.h"

#include <iostream>
#include <list>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace Freefoil {

    using namespace Private;
    using boost::bad_lexical_cast;

    static value_descriptor::E_VALUE_TYPE get_greater_type(value_descriptor::E_VALUE_TYPE value_type1, value_descriptor::E_VALUE_TYPE value_type2);
    static value_descriptor::E_VALUE_TYPE get_greatest_common_type(value_descriptor::E_VALUE_TYPE value_type1, value_descriptor::E_VALUE_TYPE value_type2);
    static bool is_assignable(value_descriptor::E_VALUE_TYPE left_value_type, value_descriptor::E_VALUE_TYPE right_value_type);

    bool param_descriptors_types_equal_functor(const param_descriptor_shared_ptr_t &param_descriptor, const param_descriptor_shared_ptr_t &the_param_descriptor) {
        return 	param_descriptor->get_value_type() == the_param_descriptor->get_value_type();
    }

    bool param_descriptors_refs_equal_functor(const param_descriptor_shared_ptr_t &param_descriptor, const param_descriptor_shared_ptr_t &the_param_descriptor) {
        return 	param_descriptor->is_ref() == the_param_descriptor->is_ref();
    }

    bool function_heads_equal_functor(const function_shared_ptr_t &func, const function_shared_ptr_t &the_func) {
        return
            func->get_name() == the_func->get_name()
            &&  func->get_param_descriptors().size() == the_func->get_param_descriptors().size()
            &&  std::equal(
                func->get_param_descriptors().begin(), func->get_param_descriptors().end(),
                the_func->get_param_descriptors().begin(),
                &param_descriptors_types_equal_functor
            );
    }

    //TODO: optimize
    int tree_analyzer::find_function(const std::string &call_name, const std::vector<value_descriptor::E_VALUE_TYPE> &invoke_args, const function_shared_ptr_list_t &funcs) const {

        int invoke_args_count = invoke_args.size();

        function_shared_ptr_list_t candidates_funcs;
        std::remove_copy_if(funcs.begin(),
                            funcs.end(),
                            std::back_inserter(candidates_funcs),
                            boost::bind(&function_descriptor::get_name, _1) != call_name or
                            boost::bind(&function_descriptor::get_param_descriptors_count, _1) != invoke_args_count
                           );

        for (function_shared_ptr_list_t::iterator cur_iter = candidates_funcs.begin(); cur_iter != candidates_funcs.end();  ) {

            assert(invoke_args_count == (*cur_iter)->get_param_descriptors_count());
            const param_descriptors_shared_ptr_list_t &params_list = (*cur_iter)->get_param_descriptors();
            bool is_valid = true;
            for (int j = 0; j < invoke_args_count; ++j) {
                if (!is_assignable(params_list[j]->get_value_type(), invoke_args[j])) {
                    is_valid = false;
                    break;
                }
            }
            if (is_valid) {
                ++cur_iter;
            } else {
                cur_iter = candidates_funcs.erase(cur_iter);
            }
        }

        if (candidates_funcs.size() != 1) {
            return -1; //mark error
        } else {
            return std::distance(funcs.begin(),
                                 std::find(funcs.begin(), funcs.end(), candidates_funcs.front())
                                );
        }
    }

    bool function_has_no_body_functor(const function_shared_ptr_t &the_func) {
        return 	!the_func->has_body();
    }

    bool param_descriptor_has_name_functor(const param_descriptor_shared_ptr_t &the_param_descriptor, const std::string &the_name) {
        return the_param_descriptor->get_name() == the_name;
    }

    bool has_complete_returns(const iter_t &iter) {

        bool result = false;

        switch (iter->value.id().to_long()) {
        case freefoil_grammar::if_branch_ID:
        case freefoil_grammar::elsif_branch_ID:
        case freefoil_grammar::else_branch_ID:
        case freefoil_grammar::block_ID:
        case freefoil_grammar::func_body_ID:
            for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
                result = result or has_complete_returns(cur_iter);
            }
            break;
        case freefoil_grammar::if_stmt_ID:
            if (iter->children.rbegin()->value.id() == freefoil_grammar::else_branch_ID) {
                for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
                    if (cur_iter->value.id() != freefoil_grammar::else_branch_ID) {
                        assert(cur_iter->value.id() == freefoil_grammar::if_branch_ID or cur_iter->value.id() == freefoil_grammar::elsif_branch_ID);
                        result = has_complete_returns(cur_iter->children.begin() + 1);
                    } else {
                        result = has_complete_returns(cur_iter->children.begin());
                    }
                    if (!result) {
                        break;
                    }
                }
            }
            break;
        case freefoil_grammar::return_stmt_ID:
            result = result or !iter->children.empty();
            break;
        default:
            break;

        }

        return result;
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

        //now we have all function declarations valid
        //some of them might contains no iterator to impl (due to errors of having only func decl in the program source)
        //it is a time for parsing valid function's impls
        for (function_shared_ptr_list_t::const_iterator cur_iter = funcs_list_.begin(), iter_end = funcs_list_.end(); cur_iter != iter_end; ++cur_iter) {
            curr_parsing_function_ = *cur_iter;
            if (curr_parsing_function_->has_body()) {
                parse_func_body(curr_parsing_function_->get_body());
            }
        }

        //now we have all user-defined functions parsed
        function_shared_ptr_list_t::iterator  iter_begin = funcs_list_.begin(), iter_end = funcs_list_.end();
        function_shared_ptr_list_t::iterator cur_iter = iter_begin;
        do {
            cur_iter = std::find_if(
                           cur_iter,
                           iter_end,
                           boost::bind(&function_has_no_body_functor, _1));
            if (cur_iter != iter_end) {
                print_error("function " + (*cur_iter)->get_name() + " is not implemented");
                ++errors_count_;

                ++cur_iter;
            }
        } while (cur_iter != iter_end);

        //check that there's only one entry point function
        std::size_t entry_points_count = 0;
        cur_iter = iter_begin;
        do {
            cur_iter = std::find_if(
                           cur_iter,
                           iter_end,
                           boost::bind(&entry_point_functor, _1));
            if (cur_iter != iter_end) {
                ++entry_points_count;
                ++cur_iter;
            }
        } while (cur_iter != iter_end);

        if (entry_points_count == 0) {
            print_error("entry point function not declared");
            ++errors_count_;
        } else if (entry_points_count > 1) {
            print_error("unable to overload entry point function");
            ++errors_count_;
        }

        //TODO: add check that each func impl has "return stmt" in all "key points"
        for (cur_iter = iter_begin; cur_iter != iter_end; ++cur_iter) {
            curr_parsing_function_ = *cur_iter;
            if (curr_parsing_function_->has_body()) {
                bool complete_returns_ways = false;
                //parse_func_body(curr_parsing_function_->get_body());
                if (curr_parsing_function_->get_type() == value_descriptor::voidType) {
                    complete_returns_ways = true;
                } else {
                    complete_returns_ways = has_complete_returns(curr_parsing_function_->get_body());
                }
                if (!complete_returns_ways) {
                    print_error("function " + curr_parsing_function_->get_name() + " has incomplete returns");
                    ++errors_count_;
                }
            }
        }

        //TODO: and other checks

        if (funcs_list_.size() >= Runtime::max_long_value) {
            print_error("user functions limit exceeded");
            ++errors_count_;
        }

        std::cout << "errors: " << errors_count_ << std::endl;
        std::cout << "analyze end" << std::endl;

        return errors_count_ == 0;
    }

    const function_shared_ptr_list_t &tree_analyzer::get_parsed_funcs_list() const {

        assert(errors_count_ == 0);
        return funcs_list_;
    }

    const constants_pool &tree_analyzer::get_parsed_constants_pool() const {

        assert(errors_count_ == 0);
        return constants_pool_;
    }

    void tree_analyzer::setup_core_funcs() {

        //TODO: populate core_funcs_list_ with core functions
        param_descriptors_shared_ptr_list_t param_descriptors;
        param_descriptors.push_back(param_descriptor_shared_ptr_t(new param_descriptor(value_descriptor::intType, false, "i")));
        core_funcs_list_.push_back(function_shared_ptr_t (new function_descriptor("foo", value_descriptor::voidType, param_descriptors)));
    }

    tree_analyzer::tree_analyzer() :errors_count_(0), symbols_handler_(NULL), curr_parsing_function_() {
        setup_core_funcs();
    }

    void tree_analyzer::parse_script(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::script_ID);

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
        create_attributes(iter, parsed_func->get_type());

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
        std::string val_name("");

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
        return param_descriptor_shared_ptr_t(new param_descriptor(val_type, ++args_count_, val_name, is_ref));
    }

    param_descriptors_shared_ptr_list_t tree_analyzer::parse_func_param_descriptors_list(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::params_list_ID);

        args_count_ = 0;
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

        param_descriptors_shared_ptr_list_t param_descriptors_list = parse_func_param_descriptors_list(iter->children.begin()+2);

        if ((signed int) param_descriptors_list.size() >= Runtime::max_byte_value) {
            print_error(iter->children.begin()+2, "function params limit exceeded");
            ++errors_count_;
        }

        const function_shared_ptr_t parsed_func = function_shared_ptr_t(new function_descriptor(parse_str(iter->children.begin()+1), func_type, param_descriptors_list));

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

                ++locals_count_;

                if (!symbols_handler_->insert(var_name, value_descriptor(var_type, -locals_count_))) {
                    print_error(cur_iter->children.begin()->children.begin(), "redeclaration of variable " + var_name);
                    ++errors_count_;
                }

                if (curr_parsing_function_->get_locals_count() >= Runtime::max_byte_value) {
                    print_error(cur_iter->children.begin()->children.begin(), "local variables limit exceeded");
                    ++errors_count_;
                } else {
                    curr_parsing_function_->inc_locals_count();
                }

                create_attributes(cur_iter->children.begin()->children.begin(), var_type, -locals_count_);

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
            } else {
                assert(cur_iter->children.begin()->value.id() == freefoil_grammar::ident_ID);

                ++locals_count_;

                if (curr_parsing_function_->get_locals_count() >= Runtime::max_byte_value) {
                    print_error(cur_iter->children.begin()->children.begin(), "local variables limit exceeded");
                    ++errors_count_;
                } else {
                    curr_parsing_function_->inc_locals_count();
                }

                const std::string var_name(parse_str(cur_iter->children.begin()));
                symbols_handler_->insert(var_name, value_descriptor(var_type, -locals_count_));
                create_attributes(cur_iter->children.begin(), var_type, -locals_count_);
            }
        }
    }

    void tree_analyzer::parse_block(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::block_ID);

        const Runtime::BYTE locals_count = locals_count_;
        symbols_handler_->scope_begin();

        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            parse_stmt(cur_iter);
        }

        symbols_handler_->scope_end();
        locals_count_ = locals_count;
    }

    void tree_analyzer::parse_stmt(const iter_t &iter) {

        switch (iter->value.id().to_long()) {
        case freefoil_grammar::block_ID: {
            parse_block(iter);
            break;
        }
        case freefoil_grammar::var_declare_stmt_list_ID: {
            parse_var_declare_stmt_list(iter);
            break;
        }
        case freefoil_grammar::return_stmt_ID: {
            parse_return_stmt(iter);
            break;
        }
        case freefoil_grammar::stmt_end_ID: {
            break;
        }
        case freefoil_grammar::func_call_ID: {
            parse_func_call(iter);
            break;
        }
        case freefoil_grammar::if_stmt_ID: {
            parse_if_stmt(iter);
            break;
        }
        //TODO: check for other stmts
        default: {
            break;
        }
        }
    }

    void tree_analyzer::parse_return_stmt(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::return_stmt_ID);

        assert(iter->children.empty() or iter->children.size() == 1);

        value_descriptor::E_VALUE_TYPE func_type = curr_parsing_function_->get_type();

        if (iter->children.empty()) {
            if (func_type != value_descriptor::voidType) {
                print_error(iter, "unable to return value from void function");
                ++errors_count_;
            } else {
                create_attributes(iter, func_type);
            }
        } else {
            if (func_type != value_descriptor::voidType) {
                assert(iter->children.size() == 1);
                assert(iter->children.begin()->value.id() == freefoil_grammar::bool_expr_ID);
                parse_bool_expr(iter->children.begin());
                value_descriptor::E_VALUE_TYPE expr_val_type = iter->children.begin()->value.value().get_value_type();
                if (is_assignable(func_type, expr_val_type)) {
                    if (func_type != expr_val_type) {
                        //make implicit cast explicit
                        create_cast(iter->children.begin(), func_type);
                    }
                    create_attributes(iter, func_type);
                } else {
                    print_error(iter->children.begin(), "cannot assign " + type_to_string(expr_val_type) + " to " + type_to_string(func_type));
                    ++errors_count_;
                }
            } else {
                print_error(iter, "unable to return value from void function");
                ++errors_count_;
            }
        }
    }

    void tree_analyzer::parse_or_xor_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::or_xor_op_ID);

        iter_t left_iter = iter->children.begin();
        iter_t right_iter = left_iter + 1;

        if (left_iter->value.id() == freefoil_grammar::or_xor_op_ID) {
            parse_or_xor_op(left_iter);
            create_attributes(iter, left_iter->value.value().get_value_type());
        } else {
            parse_bool_term(left_iter);
        }

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

    void tree_analyzer::parse_and_op(const iter_t &iter) {

        assert(parse_str(iter) == "and");

        iter_t left_iter = iter->children.begin();
        iter_t right_iter = left_iter + 1;

        if (parse_str(left_iter) == "and") {
            parse_and_op(left_iter);
            create_attributes(iter, left_iter->value.value().get_value_type());
        } else {
            parse_bool_factor(left_iter);
        }

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


    void tree_analyzer::parse_plus_minus_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::plus_minus_op_ID);

        iter_t left_iter = iter->children.begin();
        iter_t right_iter = left_iter + 1;

        if (left_iter->value.id() == freefoil_grammar::plus_minus_op_ID) {
            parse_plus_minus_op(left_iter);
            create_attributes(iter, left_iter->value.value().get_value_type());
        } else {
            parse_term(left_iter);
        }

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

    void tree_analyzer::parse_mult_divide_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::mult_divide_op_ID);

        iter_t left_iter = iter->children.begin();
        iter_t right_iter = left_iter + 1;

        if (left_iter->value.id() == freefoil_grammar::mult_divide_op_ID) {
            parse_mult_divide_op(left_iter);
            create_attributes(iter, left_iter->value.value().get_value_type());
        } else {
            parse_factor(left_iter);
        }

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

    void tree_analyzer::parse_cmp_op(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::cmp_op_ID);

        iter_t left_iter = iter->children.begin();
        iter_t right_iter = left_iter + 1;

        if (left_iter->value.id() == freefoil_grammar::cmp_op_ID) {
            parse_cmp_op(left_iter);
            create_attributes(iter, left_iter->value.value().get_value_type());
        } else {
            parse_expr(left_iter);
        }

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

    void tree_analyzer::parse_bool_expr(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_expr_ID);

        const parser_id id = iter->children.begin()->value.id();
        if (id == freefoil_grammar::bool_term_ID) {
            parse_bool_term(iter->children.begin());
        } else {
            assert(id == freefoil_grammar::or_xor_op_ID);
            parse_or_xor_op(iter->children.begin());
        }
        create_attributes(iter, iter->children.begin()->value.value().get_value_type());
    }

    void tree_analyzer::parse_expr(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::expr_ID);

        const bool has_unary_plus_minus_op = iter->children.begin()->value.id() == freefoil_grammar::unary_plus_minus_op_ID;
        if (has_unary_plus_minus_op) {
            const parser_id id = (iter->children.begin() + 1)->value.id();
            if (id == freefoil_grammar::term_ID) {
                parse_term(iter->children.begin() + 1);
            } else {
                assert(id == freefoil_grammar::plus_minus_op_ID);
                parse_plus_minus_op(iter->children.begin() + 1);
            }
            create_attributes(iter, (iter->children.begin() + 1)->value.value().get_value_type());
        } else {
            const parser_id id = iter->children.begin()->value.id();
            if (id == freefoil_grammar::term_ID) {
                parse_term(iter->children.begin());
            } else {
                assert(id == freefoil_grammar::plus_minus_op_ID);
                parse_plus_minus_op(iter->children.begin());
            }
            create_attributes(iter, iter->children.begin()->value.value().get_value_type());
        }
    }

    void tree_analyzer::parse_term(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::term_ID);

        if (iter->children.begin()->value.id() == freefoil_grammar::factor_ID) {
            parse_factor(iter->children.begin());
        } else {
            assert(iter->children.begin()->value.id() == freefoil_grammar::mult_divide_op_ID);
            parse_mult_divide_op(iter->children.begin());
        }
        create_attributes(iter, iter->children.begin()->value.value().get_value_type());
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
                stack_offset = -1;
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
            break;
        default:
            break;
        }

        create_attributes(iter, iter->children.begin()->value.value().get_value_type());
    }

    void tree_analyzer::parse_if_stmt(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::if_stmt_ID);
        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            if (cur_iter->value.id() != freefoil_grammar::else_branch_ID) {
                assert(cur_iter->value.id() == freefoil_grammar::if_branch_ID or cur_iter->value.id() == freefoil_grammar::elsif_branch_ID);
                parse_bool_expr(cur_iter->children.begin());
                parse_block(cur_iter->children.begin() + 1);
            } else {
                parse_block(cur_iter->children.begin());
            }
        }
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

        int result = find_function(func_name, invoked_value_types, funcs_list_);
        if (result != -1) {
            create_attributes(iter, funcs_list_[result]->get_type(), result);
        } else {
            if ((result = find_function(func_name, invoked_value_types, core_funcs_list_)) != -1) {
                create_attributes(iter, core_funcs_list_[result]->get_type(), result);
            } else {
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

        try {
            if (number_as_str.find('.') != std::string::npos) {
                //it is float value
                std::size_t index = constants_pool_.add_float_constant(boost::lexical_cast<float>(number_as_str));
                create_attributes(iter, value_descriptor::floatType, index);
                if (index >= Runtime::max_word_value) {
                    print_error(iter, "int values limit exceeded");
                    ++errors_count_;
                }
            } else {
                //it is int value
                std::size_t index = constants_pool_.add_int_constant(boost::lexical_cast<int>(number_as_str));
                create_attributes(iter, value_descriptor::intType, index);
                if (index >= Runtime::max_word_value) {
                    print_error(iter, "float values limit exceeded");
                    ++errors_count_;
                }
            }
        } catch (const bad_lexical_cast &e) {
            print_error(iter, "wrong value");
            ++errors_count_;
        }
    }

    void tree_analyzer::parse_quoted_string(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::quoted_string_ID);
        const std::string quoted_string(parse_str(iter));
        const std::string str_value_without_quotes(quoted_string.begin() + 1, quoted_string.end() - 1);

        const std::size_t index = constants_pool_.add_string_constant(str_value_without_quotes);
        create_attributes(iter, value_descriptor::stringType, index);

        if (index >= Runtime::max_word_value) {
            print_error(iter, "string values limit exceeded");
            ++errors_count_;
        }
    }

    void tree_analyzer::parse_func_body(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_body_ID);
        locals_count_ = 0;

        symbols_handler_.reset(new symbols_handler);
        symbols_handler_->scope_begin();

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
    static value_descriptor::E_VALUE_TYPE get_greater_type(value_descriptor::E_VALUE_TYPE value_type1, value_descriptor::E_VALUE_TYPE value_type2) {

        if (value_type1 == value_type2) {
            return value_type1;
        }
        if (value_type1 == value_descriptor::stringType and
                (value_type2 == value_descriptor::intType or value_type2 == value_descriptor::boolType)) {
            return value_descriptor::stringType;
        }/*
        if (value_type1 == value_descriptor::intType and value_type2 == value_descriptor::floatType) {
            return value_descriptor::floatType;
        }*/
        if (value_type1 == value_descriptor::floatType and value_type2 == value_descriptor::intType) {
            return value_descriptor::floatType;
        }
        if (value_type1 == value_descriptor::intType and value_type2 == value_descriptor::boolType) {
            return value_descriptor::intType;
        }
        if (value_type1 == value_descriptor::floatType and value_type2 == value_descriptor::boolType) {
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

        return get_greater_type(left_value_type, right_value_type) != value_descriptor::undefinedType;
    }

//possible implicit casts:
    /*
    str   <-- int
    float <-  int
    str   <-- bool

    int   <-- bool
    float <-- bool
    */
}
