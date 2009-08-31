#include "script.h"
#include "freefoil_grammar.h"
#include "freefoil_defs.h"
#include "exceptions.h"

#include <iostream>
#include <list>
#include <algorithm>
#include <boost/bind.hpp>

#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <map>
#endif

namespace Freefoil {

    using namespace Private;

    bool params_types_equal_functor(const param_shared_ptr_t &param, const param_shared_ptr_t &the_param) {
        return 	param->get_value_type() == the_param->get_value_type();
    }

    bool params_refs_equal_functor(const param_shared_ptr_t &param, const param_shared_ptr_t &the_param) {
        return 	param->is_ref() == the_param->is_ref();
    }

    bool function_heads_equal_functor(const function_shared_ptr_t &func, const function_shared_ptr_t &the_func) {
        return 		func->get_type() == the_func->get_type()
                 &&  func->get_name() == the_func->get_name()
                 &&  func->get_params().size() == the_func->get_params().size()
                 &&  std::equal(
                     func->get_params().begin(), func->get_params().end(),
                     the_func->get_params().begin(),
                     &params_types_equal_functor);
    }

    bool function_has_no_body_functor(const function_shared_ptr_t &the_func) {
        return 	!the_func->has_body();
    }

    bool entry_point_functor(const function_shared_ptr_t &the_func) {
        return 	the_func->get_name() == "main";
    }

    void script::exec() {

        freefoil_grammar grammar;
        std::string str;
        while (getline(std::cin, str)) {
            if (str == "q") {
                break;
            }

            iterator_t iter_begin = str.begin();
            iterator_t iter_end = str.end();

            tree_parse_info_t info = ast_parse(iter_begin, iter_end, grammar, space_p);
            if (info.full) {
                std::cout << "succeeded" << std::endl;

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
                rule_names[freefoil_grammar::params_list_ID] = "params_list";
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

                tree_to_xml(std::cerr,
                            info.trees,
                            "",
                            rule_names);
#endif

                parse(info.trees.begin());
            } else {
                std::cout << "failed" << std::endl;
                std::cout << "stopped at: " << *(info.stop) << std::endl;
            }
        }
    }

    void script::parse(const iter_t &iter) {
        try {
            std::cout << "parsing begin" << std::endl;
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
                throw freefoil_exception("function " + (*invalid_function_iter)->get_name() + " is not implemented");
            }

            if (std::find_if(
                        funcs_list_.begin(),
                        funcs_list_.end(),
                        boost::bind(&entry_point_functor, _1)) == funcs_list_.end()) {
                throw freefoil_exception("entry point not declared");
            }

            //now we have all function declarations valid and we are sure we have "main" entry point function
            //it is a time for parsing function's bodies and generate intermediate code
            for (function_shared_ptr_list_t::const_iterator cur_iter = funcs_list_.begin(), iter_end = funcs_list_.end(); cur_iter != iter_end; ++cur_iter) {
                parse_func_body((*cur_iter)->get_body());
            }

            //TODO:
        } catch (const freefoil_exception &e) {
            std::cout << "catched: " << e.what() << std::endl;
        }

        std::cout << "parsing end" << std::endl;
    }

    script::script()  {
        params_shared_ptr_list_t params;
        params.push_back(param_shared_ptr_t(new param(value_descriptor::intType, false, "i")));
        core_funcs_list_.push_back(function_shared_ptr_t (new function_descriptor("foo", function_descriptor::voidType, params)));
        //	core_funcs_list_.push_back(function_shared_ptr_t(new function_descriptor("foo", function::voidType)));
        //TODO: add all core funcs
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
                throw freefoil_exception("function " + old_func->get_name() + " already implemented");
            }
            //if that old function differs from our new one only by some "refs", spit an error
            assert(parsed_func->get_params().size() == old_func->get_params().size());
            if (!std::equal(
                        old_func->get_params().begin(), old_func->get_params().end(),
                        parsed_func->get_params().begin(),
                        &params_refs_equal_functor)) {
                throw freefoil_exception("previous declaration differs from this one only by \"ref(s)\"");
            }
            old_func->set_body(iter->children.begin()+1);
        } else {
            //it is completely new function
            parsed_func->set_body(iter->children.begin()+1);
            funcs_list_.push_back(parsed_func);
        }
    }

    param_shared_ptr_t script::parse_func_param(const iter_t &iter) {
        assert(iter->value.id() == freefoil_grammar::param_ID);

        value_descriptor::E_VALUE_TYPE val_type;
        bool is_ref = false;
        std::string val_name = "";

        const std::string val_type_as_str(parse_str(iter->children.begin()));

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
        return param_shared_ptr_t(new param(val_type, is_ref, val_name));
    }

    params_shared_ptr_list_t script::parse_func_params_list(const iter_t &iter) {
        assert(iter->value.id() == freefoil_grammar::params_list_ID);
        params_shared_ptr_list_t params_list;
        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            params_list.push_back(parse_func_param(cur_iter));
        }
        return params_list;
    }

    function_shared_ptr_t script::parse_func_head(const iter_t &iter) {

        assert(iter->value.id() == freefoil_grammar::func_head_ID);
        assert(iter->children.size() == 3);
        assert((iter->children.begin())->value.id() == freefoil_grammar::func_type_ID);
        function_descriptor::E_FUNCTION_TYPE func_type;

        const std::string func_type_as_str(parse_str(iter->children.begin()));
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

        const function_shared_ptr_t parsed_func = function_shared_ptr_t(new function_descriptor(parse_str(iter->children.begin()+1), func_type, parse_func_params_list(iter->children.begin()+2)));

        if (std::find_if(
                    core_funcs_list_.begin(),
                    core_funcs_list_.end(),
                    boost::bind(&function_heads_equal_functor, _1, parsed_func)) != core_funcs_list_.end()) {
            throw freefoil_exception("unable to override core function");
        }

        return parsed_func;
    }

    void script::parse_stmt(const iter_t &iter) {

        const parser_id id = iter->value.id();
        switch (id.to_long()) {
        case freefoil_grammar::block_ID:{
            scope_stack_.begin_scope();
            parse_stmt(iter->children.begin());
            scope_stack_.end_scope();
            break;
        }
        case freefoil_grammar::var_declare_stmt_list_ID:{
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
            for (iter_t cur_iter = iter->children.begin() + 1, iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter){
                //parse var decl
                const std::string var_name(parse_str(cur_iter->children.begin()));
                const std::size_t bucket_index = symbol_table_.insert(var_name, value_descriptor(var_type));
                scope_stack_.push_bucket_index(bucket_index);
                //TODO: generate an instruction
            }
            break;
        }
            //TODO: check for other stmts
        default:
            break;
        }
    }

    void script::parse_func_body(const iter_t &iter) {
        symbol_table_ = symbol_table();
        scope_stack_.attach_symbol_table(&symbol_table_);

        assert(iter->value.id() == freefoil_grammar::func_body_ID);
        for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter) {
            parse_stmt(cur_iter);
        }
    }

    std::string script::parse_str(const iter_t &iter) {
        return std::string(iter->value.begin(), iter->value.end());
    }
}
