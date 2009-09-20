#include "script.h"
#include "freefoil_grammar.h"
#include "AST_defs.h"
#include "exceptions.h"
#include "opcodes.h"

#include <iostream>
#include <list>
#include <algorithm>
#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <map>
#endif
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace Freefoil {

    using namespace Private;

    static tree_parse_info_t build_AST(const iterator_t &iter_begin, const iterator_t &iter_end);
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

    bool function_has_no_body_functor(const function_shared_ptr_t &the_func) {
        return 	!the_func->has_body();
    }

    bool entry_point_functor(const function_shared_ptr_t &the_func) {
        return 	the_func->get_name() == "main";
    }

    bool param_descriptor_has_name_functor(const param_descriptor_shared_ptr_t &the_param_descriptor, const std::string &the_name) {
        return the_param_descriptor->get_name() == the_name;
    }


    void script::exec() {

        std::string str;
        while (getline(std::cin, str)) {
            if (str == "q") {
                break;
            }
            if (!funcs_list_.empty()) {
                funcs_list_.clear();
            }

            try {

                std::cout << "parsing begin" << std::endl;
                tree_parse_info_t info = build_AST(iterator_t(str.begin(), str.end()), iterator_t());
                std::cout << "parsing end" << std::endl;

#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
                // dump parse tree as XML
                std::map<parser_id, std::string> rule_names;
                rule_names[freefoil_grammar::script_ID] = "script";
                rule_names[freefoil_grammar::func_decl_ID] = "func_decl";
                rule_names[freefoil_grammar::func_impl_ID] = "func_impl";
                rule_names[freefoil_grammar::ident_ID] = "ident";
                rule_names[freefoil_grammar::stmt_end_ID] = "stmt_end";
                rule_names[freefoil_grammar::func_head_ID] = "func_head";
                rule_names[freefoil_grammar::func_body_ID] = "func_body";
                rule_names[freefoil_grammar::params_list_ID] = "params_list";
                rule_names[freefoil_grammar::param_ID] = "param";
                rule_names[freefoil_grammar::ref_ID] = "ref";
                rule_names[freefoil_grammar::func_type_ID] = "func_type";
                rule_names[freefoil_grammar::stmt_ID] = "stmt";
                rule_names[freefoil_grammar::var_declare_stmt_list_ID] = "var_declare_stmt_list";
                rule_names[freefoil_grammar::expr_ID] = "expr";
                rule_names[freefoil_grammar::term_ID] = "term";
                rule_names[freefoil_grammar::factor_ID] = "factor";
                rule_names[freefoil_grammar::expr_ID] = "expr";
                rule_names[freefoil_grammar::number_ID] = "number";
                rule_names[freefoil_grammar::bool_expr_ID] = "bool_expr";
                rule_names[freefoil_grammar::bool_term_ID] = "bool_term";
                rule_names[freefoil_grammar::bool_factor_ID] = "bool_factor";
                rule_names[freefoil_grammar::bool_relation_ID] = "bool_relation";
                rule_names[freefoil_grammar::quoted_string_ID] = "quoted_string";
                rule_names[freefoil_grammar::func_call_ID] = "func_call";
                rule_names[freefoil_grammar::invoke_args_list_ID] = "invoke_args_list";
                rule_names[freefoil_grammar::block_ID] = "block";
                rule_names[freefoil_grammar::var_declare_tail_ID] = "var_declare_tail";
                rule_names[freefoil_grammar::assign_op_ID] = "assign_op";
                rule_names[freefoil_grammar::cmp_op_ID] = "cmp_op";
                rule_names[freefoil_grammar::or_tail_ID] = "or_tail";
                rule_names[freefoil_grammar::and_tail_ID] = "and_tail";
                rule_names[freefoil_grammar::relation_tail_ID] = "relation_tail";
                rule_names[freefoil_grammar::plus_minus_op_ID] = "plus_minus_op";
                rule_names[freefoil_grammar::mult_divide_op_ID] = "mult_divide_op";
                rule_names[freefoil_grammar::bool_constant_ID] = "bool_constant";

                tree_to_xml(std::cerr,
                            info.trees,
                            "",
                            rule_names);
#endif
                parse(info.trees.begin());
            } catch (const freefoil_grammar::parser_error_t &e) {
                std::string error_msg;

                switch (e.descriptor) {
                case Private::bool_expr_expected_error:
                    error_msg = "bool expression expected";
                    break;
                case Private::bool_factor_expected_error:
                    error_msg = "bool factor expected";
                    break;
                case Private::closed_block_expected_error:
                    error_msg = "closed block expected";
                    break;
                case Private::closed_bracket_expected_error:
                    error_msg = "closed bracket expected";
                    break;
                case Private::data_expected_error:
                    error_msg = "unexpected end";
                    break;
                case Private::expr_expected_error:
                    error_msg = "expression expected";
                    break;
                case Private::factor_expected_error:
                    error_msg = "factor expected";
                    break;
                case Private::ident_expected_error:
                    error_msg = "identificator expected";
                    break;
                case Private::open_block_expected_error:
                    error_msg = "open block expected";
                    break;
                case Private::open_bracket_expected_error:
                    error_msg = "open bracket expected";
                    break;
                case Private::relation_expected_error:
                    error_msg = "relation expected";
                    break;
                case Private::stmt_end_expected_error:
                    error_msg = "statement end expected";
                    break;
                case Private::term_expected_error:
                    error_msg = "term expected error";
                    break;
                default:
                    error_msg = "unknown parse error";
                    break;
                }

                const iterator_t iter = e.where;
                std::cout << "[" << iter.get_position().line << ":" << iter.get_position().column << "] ";
                std::cout << error_msg << std::endl;
            }
        }
    }

    void script::parse(const iter_t &iter) {

        std::cout << "analyze begin" << std::endl;

        errors_count_ = 0;

       // try {
            const parser_id id = iter->value.id();
            assert(id == freefoil_grammar::script_ID || id == freefoil_grammar::func_decl_ID || id == freefoil_grammar::func_impl_ID);
            switch (id.to_long()) {
            case freefoil_grammar::script_ID:
                parse_script(iter);
                break;
            case freefoil_grammar::func_decl_ID:
                parse_func_decl(iter);
                break;
            case freefoil_grammar::func_impl_ID:
                parse_func_impl(iter);
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
                    curr_parsing_function_->print_bytecode_stream();
                }
            }

            //TODO:
            //
        /*} catch (const freefoil_exception &e) {
            std::cout << "catched: " << e.what() << std::endl;
        }*/

        std::cout << "errors: " << errors_count_ << std::endl;
        std::cout << "analyze end" << std::endl;
    }

    void script::setup_core_funcs() {

        //TODO: populate core_funcs_list_ with core functions
        param_descriptors_shared_ptr_list_t param_descriptors;
        param_descriptors.push_back(param_descriptor_shared_ptr_t(new param_descriptor(value_descriptor::intType, false, "i")));
        core_funcs_list_.push_back(function_shared_ptr_t (new function_descriptor("foo", function_descriptor::voidType, param_descriptors)));
    }

    script::script() :errors_count_(0), symbols_handler_(NULL), curr_parsing_function_(), stack_offset_(0) {
        setup_core_funcs();
    }

    void script::parse_script(const iter_t &iter) {

        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            if (cur_iter->value.id() == freefoil_grammar::func_decl_ID) {
                parse_func_decl(cur_iter);
            } else {
                assert(cur_iter->value.id() == freefoil_grammar::func_impl_ID);
                parse_func_impl(cur_iter);
            }
        }
    }

    void script::parse_func_decl(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_decl_ID);

        stack_offset_ = 0;

        const function_shared_ptr_t parsed_func = parse_func_head(iter->children.begin());

        //is it a redeclaration of function?
        if (std::find_if(
                    funcs_list_.begin(),
                    funcs_list_.end(),
                    boost::bind(&function_heads_equal_functor, _1, parsed_func)) != funcs_list_.end()) {
            print_error(iter->children.begin(), "function " + parsed_func->get_name() + " already declared");
            ++errors_count_;
        }

        assert( (iter->children.begin() + 1)->value.id() == freefoil_grammar::stmt_end_ID);
        funcs_list_.push_back(parsed_func);
    }

    void script::parse_func_impl(const iter_t &iter) {

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
                print_error("function " + old_func->get_name() + " already implemented");
                ++errors_count_;
            }
            //if that old function differs from our new one only by some "refs", spit an error
            assert(parsed_func->get_param_descriptors().size() == old_func->get_param_descriptors().size());
            if (!std::equal(
                        old_func->get_param_descriptors().begin(), old_func->get_param_descriptors().end(),
                        parsed_func->get_param_descriptors().begin(),
                        &param_descriptors_refs_equal_functor)) {
                print_error("previous decl differs from this one only by ref(s)");
                ++errors_count_;
            }
            old_func->set_body(iter->children.begin()+1);
        } else {
            //it is completely new function
            parsed_func->set_body(iter->children.begin()+1);
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

    param_descriptors_shared_ptr_list_t script::parse_func_param_descriptors_list(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::params_list_ID);
        param_descriptors_shared_ptr_list_t param_descriptors_list;
        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            param_descriptors_list.push_back(parse_func_param_descriptor(cur_iter));
        }
        return param_descriptors_list;
    }

    function_shared_ptr_t script::parse_func_head(const iter_t &iter) {

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

        const function_shared_ptr_t parsed_func = function_shared_ptr_t(new function_descriptor(parse_str(iter->children.begin()+1), func_type, parse_func_param_descriptors_list(iter->children.begin()+2)));

        if (std::find_if(
                    core_funcs_list_.begin(),
                    core_funcs_list_.end(),
                    boost::bind(&function_heads_equal_functor, _1, parsed_func)) != core_funcs_list_.end()) {
            print_error("unable to override core function " + parsed_func->get_name());
            ++errors_count_;
        }

        return parsed_func;
    }

    void script::parse_stmt(const iter_t &iter) {

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

                //parse var declare tails
                assert(cur_iter->value.id() == freefoil_grammar::var_declare_tail_ID);
                const std::string var_name(parse_str(cur_iter->children.begin()));

                symbols_handler_->insert(var_name, value_descriptor(var_type, stack_offset_));

                if (cur_iter->children.begin() + 1 != cur_iter->children.end()) {
                    //it is an assign expr
                    const value_descriptor::E_VALUE_TYPE value_type2 = parse_bool_expr(cur_iter->children.begin() + 2);
                    if (var_type != value_type2) {
                        if (var_type == value_descriptor::intType) {
                            if (value_type2 == value_descriptor::floatType) {
                                std::cout << "attention: casting from float to int";
                            } else if (value_type2 == value_descriptor::boolType) {
                                ;
                            } else {
                                throw freefoil_exception("impossible operation");
                            }
                        } else if (var_type == value_descriptor::floatType) {
                            if (value_type2 == value_descriptor::intType) {
                                ;
                            } else if (value_type2 == value_descriptor::boolType) {
                                ;
                            } else {
                                throw freefoil_exception("impossible operation");
                            }
                        } else {
                            throw freefoil_exception("impossible operation");
                        }
                    }

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

    value_descriptor::E_VALUE_TYPE script::parse_bool_expr(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_expr_ID);

        value_descriptor::E_VALUE_TYPE value_type1 = parse_bool_term(iter->children.begin());
        if (iter->children.begin() + 1 != iter->children.end()) {
            value_descriptor::E_VALUE_TYPE value_type2 = parse_or_tail(iter->children.begin() + 1);
            if (value_type1 != value_descriptor::boolType or value_type2 != value_descriptor::boolType) {
                throw freefoil_exception("impossible operation");
            } else {
                return value_descriptor::boolType;
            }
        } else {
            return value_type1;
        }
    }

    value_descriptor::E_VALUE_TYPE script::parse_or_tail(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::or_tail_ID);

        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {

            const std::string or_operation_as_str(parse_str(cur_iter++));
            value_descriptor::E_VALUE_TYPE value_type = parse_bool_term(cur_iter);
            if (value_type != value_descriptor::boolType) {
                throw freefoil_exception("impossible operation");
            }
            if (or_operation_as_str == "or") {
                curr_parsing_function_->add_instruction(instruction(Private::OR_OP));
            } else {
                assert(or_operation_as_str == "xor");
                curr_parsing_function_->add_instruction(instruction(Private::XOR_OP));
            }
        }
        return value_descriptor::boolType;
    }

    value_descriptor::E_VALUE_TYPE script::parse_and_tail(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::and_tail_ID);
        assert(parse_str(iter->children.begin()) == "and");

        iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end();
        while (cur_iter != iter_end && parse_str(cur_iter) == "and") {
            value_descriptor::E_VALUE_TYPE value_type = parse_bool_factor(++cur_iter);
            if (value_type != value_descriptor::boolType) {
                throw freefoil_exception("impossible operation");
            }
            curr_parsing_function_->add_instruction(instruction(Private::AND_OP));
            ++cur_iter;
        }
        return value_descriptor::boolType;
    }

    value_descriptor::E_VALUE_TYPE script::parse_expr(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::expr_ID);

        const bool has_unary_plus_minus_op = iter->children.begin()->value.id() == freefoil_grammar::plus_minus_op_ID;
        iter_t cur_iter = has_unary_plus_minus_op ? iter->children.begin() + 1 : iter->children.begin();
        const iter_t iter_end = iter->children.end();
        value_descriptor::E_VALUE_TYPE value_type1 = parse_term(cur_iter++);

        while (cur_iter != iter_end) {
            assert(cur_iter->value.id() == freefoil_grammar::plus_minus_op_ID);
            value_descriptor::E_VALUE_TYPE value_type2;
            if (parse_str(cur_iter) == "+") {
                value_type2 = parse_term(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::PLUS_OP));
            } else {
                assert(parse_str(cur_iter) == "-");
                value_type2 = parse_term(++cur_iter);
                curr_parsing_function_->add_instruction(instruction(Private::MINUS_OP));
            }
            if (value_type1 == value_descriptor::stringType and value_type2 == value_descriptor::stringType) {
                ;
            } else {
                if (value_type1 == value_descriptor::intType) {
                    if (value_type2 == value_descriptor::intType) {
                        ;
                    } else if (value_type2 == value_descriptor::floatType) {
                        value_type1 = value_descriptor::floatType;
                    } else {
                        throw freefoil_exception("impossible operation");
                    }
                } else if (value_type1 == value_descriptor::floatType) {
                    if (value_type2 == value_descriptor::floatType) {
                        ;
                    } else if (value_type2 == value_descriptor::intType) {
                        ;
                    } else {
                        throw freefoil_exception("impossible operation");
                    }
                } else {
                    throw freefoil_exception("impossible operation");
                }
            }

            ++cur_iter;
        }

        if (has_unary_plus_minus_op) {
            if (value_type1 != value_descriptor::intType and value_type1 != value_descriptor::floatType) {
                throw freefoil_exception("impossible operation");
            }
            if (parse_str(iter->children.begin()) == "-") {
                curr_parsing_function_->add_instruction(instruction(Private::NEGATE_OP));
            }
        }

        return value_type1;
    }

    value_descriptor::E_VALUE_TYPE script::parse_term(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::term_ID);

        iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end();
        value_descriptor::E_VALUE_TYPE value_type1 = parse_factor(cur_iter++);

        while (cur_iter != iter_end) {
            assert(cur_iter->value.id() == freefoil_grammar::mult_divide_op_ID);
            value_descriptor::E_VALUE_TYPE value_type2;
            if (parse_str(cur_iter) == "*") {
                value_type2 = parse_factor(++cur_iter);
                if (value_type1 == value_descriptor::intType) {
                    if (value_type2 == value_descriptor::intType) {
                        ;
                    } else if (value_type2 == value_descriptor::floatType) {
                        value_type1 = value_descriptor::floatType;
                    } else {
                        throw freefoil_exception("impossible operation");
                    }
                } else if (value_type1 == value_descriptor::floatType) {
                    if (value_type2 == value_descriptor::intType) {
                        ;
                    } else if (value_type2 == value_descriptor::floatType) {
                        ;
                    } else {
                        throw freefoil_exception("impossible operation");
                    }
                } else {
                    throw freefoil_exception("impossible operation");
                }
                curr_parsing_function_->add_instruction(instruction(Private::MULT_OP));
            } else {
                assert(parse_str(cur_iter) == "/");
                value_type2 = parse_factor(++cur_iter);
                if (value_type1 != value_descriptor::intType and value_type1 != value_descriptor::floatType
                        and value_type2 != value_descriptor::intType and value_type2 != value_descriptor::floatType ) {
                    throw freefoil_exception("impossible operation");
                } else {
                    value_type1 = value_descriptor::floatType;
                }

                curr_parsing_function_->add_instruction(instruction(Private::DIVIDE_OP));
            }

            ++cur_iter;
        }

        return value_type1;
    }

    value_descriptor::E_VALUE_TYPE script::parse_factor(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::factor_ID);

        switch (iter->children.begin()->value.id().to_long()) {
        case freefoil_grammar::ident_ID: {

            const string name(parse_str(iter->children.begin()));
            int stack_offset;
            value_descriptor::E_VALUE_TYPE value_type1;
            value_descriptor *the_value_descriptor = symbols_handler_->lookup(name);
            if (the_value_descriptor != NULL) {
                value_type1 = the_value_descriptor->get_value_type();
                stack_offset = the_value_descriptor->get_stack_offset();
            } else {
                const param_descriptors_shared_ptr_list_t::const_iterator suitable_param_descriptor_iter
                = std::find_if(
                      curr_parsing_function_->get_param_descriptors().begin(),
                      curr_parsing_function_->get_param_descriptors().end(),
                      boost::bind(&param_descriptor_has_name_functor, _1, name));
                if (suitable_param_descriptor_iter != curr_parsing_function_->get_param_descriptors().end()) {
                    value_type1 = (*suitable_param_descriptor_iter)->get_value_type();
                    stack_offset = (*suitable_param_descriptor_iter)->get_stack_offset();
                } else {
                    //error. such variable not presented
                    throw freefoil_exception("unknown ident: " + name);
                }
            }

            curr_parsing_function_->add_instruction(instruction(Private::LOAD_VAR));
            curr_parsing_function_->add_instruction(instruction(Private::GET_VAR_INDEX, stack_offset));
            return value_type1;
        }

        case freefoil_grammar::number_ID: {
            return parse_number(iter->children.begin());
        }

        case freefoil_grammar::quoted_string_ID: {
            return parse_quoted_string(iter->children.begin());
            break;
        }

        case freefoil_grammar::bool_expr_ID:
            return parse_bool_expr(iter->children.begin());

        case freefoil_grammar::func_call_ID:
            return parse_func_call(iter->children.begin());

        case freefoil_grammar::bool_constant_ID:
            return parse_bool_constant(iter->children.begin());

        default:
            //can't occur
            throw freefoil_exception("impossible operation");
        }
    }

//TODO:
    value_descriptor::E_VALUE_TYPE script::parse_func_call(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_call_ID);

        const std::string func_name(parse_str(iter->children.begin()));
        for (iter_t cur_iter = (iter->children.begin() + 2)->children.begin(), iter_end = (iter->children.begin() + 2)->children.end(); cur_iter != iter_end; ++cur_iter) {
            //TODO:
        }
        //TODO:

        return value_descriptor::intType;
    }

    value_descriptor::E_VALUE_TYPE script::parse_bool_constant(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_constant_ID);

        const std::string boolean_constant_as_str(parse_str(iter));
        if (boolean_constant_as_str == "true") {
            curr_parsing_function_->add_instruction(instruction(Private::PUSH_TRUE));
        } else {
            assert(boolean_constant_as_str == "false");
            curr_parsing_function_->add_instruction(instruction(Private::PUSH_FALSE));
        }
        return value_descriptor::boolType;
    }

    value_descriptor::E_VALUE_TYPE script::parse_bool_term(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_term_ID);

        value_descriptor::E_VALUE_TYPE value_type1 = parse_bool_factor(iter->children.begin());
        if (iter->children.begin() + 1 != iter->children.end()) {
            value_descriptor::E_VALUE_TYPE value_type2 = parse_and_tail(iter->children.begin() + 1);
            if (value_type1 != value_descriptor::boolType or value_type2 != value_descriptor::boolType) {
                throw freefoil_exception("impossible operation");
            } else {
                return value_descriptor::boolType;
            }
        } else {
            return value_type1;
        }
    }

    value_descriptor::E_VALUE_TYPE script::parse_bool_factor(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_factor_ID);

        const bool negate = (parse_str(iter->children.begin()) == "not");
        const value_descriptor::E_VALUE_TYPE value_type = parse_bool_relation(negate ? iter->children.begin() + 1 : iter->children.begin());
        if (negate) {
            if (value_type != value_descriptor::boolType && value_type != value_descriptor::intType) {
                throw freefoil_exception("impossible operation");
            }
            curr_parsing_function_->add_instruction(instruction(Private::NOT_OP));
            return value_type;
        } else {
            return value_type;
        }
    }

    value_descriptor::E_VALUE_TYPE script::parse_bool_relation(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::bool_relation_ID);

        iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end();
        value_descriptor::E_VALUE_TYPE value_type1 = parse_expr(cur_iter++);
        if (cur_iter == iter_end) {
            return value_type1;
        } else {
            value_descriptor::E_VALUE_TYPE value_type2;
            while (cur_iter != iter_end) {
                assert(cur_iter->value.id() == freefoil_grammar::relation_tail_ID);
                const std::string relation_op_as_str(parse_str(cur_iter));

                if (relation_op_as_str == "==") {
                    value_type2 = parse_expr(++cur_iter);
                    curr_parsing_function_->add_instruction(instruction(Private::EQUAL_OP));
                } else if (relation_op_as_str == "!=") {
                    value_type2 = parse_expr(++cur_iter);
                    curr_parsing_function_->add_instruction(instruction(Private::NOT_EQUAL_OP));
                } else if (relation_op_as_str == "<=") {
                    value_type2 = parse_expr(++cur_iter);
                    curr_parsing_function_->add_instruction(instruction(Private::LESS_OR_EQUAL_OP));
                } else if (relation_op_as_str == ">=") {
                    value_type2 = parse_expr(++cur_iter);
                    curr_parsing_function_->add_instruction(instruction(Private::GREATER_OR_EQUAL_OP));
                } else if (relation_op_as_str == "<") {
                    value_type2 = parse_expr(++cur_iter);
                    curr_parsing_function_->add_instruction(instruction(Private::LESS_OP));
                } else {
                    assert(relation_op_as_str == ">");
                    value_type2 = parse_expr(++cur_iter);
                    curr_parsing_function_->add_instruction(instruction(Private::GREATER_OP));
                }

                if (value_type1 != value_type2) {
                    throw freefoil_exception("impossible operation");
                }

                value_type1 = value_descriptor::boolType;
            }

            return value_descriptor::boolType;
        }
    }

    value_descriptor::E_VALUE_TYPE script::parse_number(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::number_ID);

        const std::string number_as_str(parse_str(iter));
        if (number_as_str.find('.') != std::string::npos) {
            //it is float value
            const std::size_t index = curr_parsing_function_->add_float_constant(boost::lexical_cast<float>(number_as_str));
            curr_parsing_function_->add_instruction(instruction(Private::GET_FLOAT_CONST));
            curr_parsing_function_->add_instruction(instruction(Private::GET_INDEX_OF_CONST, (int)index));

            return value_descriptor::floatType;
        } else {
            //it is int value
            const std::size_t index = curr_parsing_function_->add_int_constant(boost::lexical_cast<int>(number_as_str));
            curr_parsing_function_->add_instruction(instruction(Private::GET_INT_CONST));
            curr_parsing_function_->add_instruction(instruction(Private::GET_INDEX_OF_CONST, (int)index));

            return value_descriptor::intType;
        }
    }

    value_descriptor::E_VALUE_TYPE script::parse_quoted_string(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::quoted_string_ID);
        const std::string quoted_string(parse_str(iter));
        const std::string str_value_without_quotes(quoted_string.begin() + 1, quoted_string.end() - 1);

        const std::size_t index = curr_parsing_function_->add_string_constant(str_value_without_quotes);
        curr_parsing_function_->add_instruction(instruction(Private::GET_STRING_CONST));
        curr_parsing_function_->add_instruction(instruction(Private::GET_INDEX_OF_CONST, (int)index));

        return value_descriptor::stringType;
    }

    void script::parse_func_body(const iter_t &iter) {

        stack_offset_ = curr_parsing_function_->get_param_descriptors_count();

        symbols_handler_.reset(new symbols_handler);
        symbols_handler_->scope_begin();

        assert(iter->value.id() == freefoil_grammar::func_body_ID);
        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            parse_stmt(cur_iter);
        }

        symbols_handler_->scope_end();
    }

    void script::print_error(const iter_t &iter, const std::string &msg){
        std::cout << "line " << iter->value.begin().get_position().line << std::endl;
        std::cout << msg << std::endl;
    }

    void script::print_error(const std::string &msg){
        std::cout << msg << std::endl;
    }



////////////////////////////////////////////////////////////////////////////////////////////////////
    static std::string parse_str(const iter_t &iter) {
        return std::string(iter->value.begin(), iter->value.end());
    }

    static tree_parse_info_t build_AST(const iterator_t &iter_begin, const iterator_t &iter_end) {
        return ast_parse<factory_t>(iter_begin, iter_end, freefoil_grammar(), space_p);
    }

    /*possible implicit casts:

                      bool -> string
                      bool -> int

                      string -> int
                      string -> bool
                      string -> float
    */
}
