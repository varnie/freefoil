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

namespace Freefoil{

	using namespace Private;

	bool params_types_equal_functor(const param_shared_ptr_t &param, const param_shared_ptr_t &the_param){
		return 	param->get_value_type() == the_param->get_value_type();
	}

	bool params_refs_equal_functor(const param_shared_ptr_t &param, const param_shared_ptr_t &the_param){
		return 	param->is_ref() == the_param->is_ref();
	}

	bool function_heads_equal_functor(const function_shared_ptr_t &func, const function_shared_ptr_t &the_func){
		return 		func->get_type() == the_func->get_type()
				&&  func->get_name() == the_func->get_name()
				&&  func->get_params().size() == the_func->get_params().size()
				&&  std::equal(
								func->get_params().begin(), func->get_params().end(),
								the_func->get_params().begin(),
								&params_types_equal_functor);
	}

	void script::exec(){

			freefoil_grammar grammar;
			std::string str;
			while (getline(std::cin, str)){
				if (str == "q"){
					break;
				}

				iterator_t iter_begin = str.begin();
				iterator_t iter_end = str.end();

				tree_parse_info_t info = ast_parse(iter_begin, iter_end, grammar, space_p);
				if (info.full){
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
        rule_names[freefoil_grammar::var_assign_stmt_list_ID] = "var_assign_stmt_list";
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


        tree_to_xml(std::cerr,
        			info.trees,
        			"",
            		rule_names);
#endif

					parse(info.trees.begin());
			}else{
					std::cout << "failed" << std::endl;
					std::cout << "stopped at: " << *(info.stop) << std::endl;
			}
		}
	}

	void script::parse(const iter_t &iter){
		try{
			std::cout << "parsing begin" << std::endl;
			const parser_id id = iter->value.id();
			std::cout << "id = " << id.to_long();
			assert(id == freefoil_grammar::script_ID || id == freefoil_grammar::func_decl_ID || id == freefoil_grammar::func_impl_ID);
			switch (id.to_long()){
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

			std::cout << "parsing end" << std::endl;
		} catch (const freefoil_exception &e){
			std::cout << "catched: " << e.what() << std::endl;
		}
	}

	script::script(){
		params_shared_ptr_list_t params;
		params.push_back(param_shared_ptr_t(new param(value::intType, false, "i")));
		core_funcs_list_.push_back(function_shared_ptr_t (new function("foo", function::voidType, params)));
	//	core_funcs_list_.push_back(function_shared_ptr_t(new function("foo", function::voidType)));
		//TODO: add all core funcs
	}

	void script::parse_script(const iter_t &iter){
		for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter){
			 if (cur_iter->value.id() == freefoil_grammar::func_decl_ID){
			 	parse_func_decl(cur_iter);
			 }else{
			 	assert(cur_iter->value.id() == freefoil_grammar::func_impl_ID);
			 	parse_func_impl(cur_iter);
			 }
		}
	}

	void script::parse_func_decl(const iter_t &iter){
		assert(iter->value.id() == freefoil_grammar::func_decl_ID);
		const function_shared_ptr_t parsed_func = parse_func_head(iter->children.begin());

		//is it a redeclaration of function?
		if (std::find_if(
			funcs_list_.begin(),
			funcs_list_.end(),
			boost::bind(&function_heads_equal_functor, _1, parsed_func)) != funcs_list_.end())
		{
			throw freefoil_exception("function " + parsed_func->get_name() + " already declared");
		}

		funcs_list_.push_back(parsed_func);

		assert( (iter->children.begin() + 1)->value.id() == freefoil_grammar::stmt_end_ID);
	}

	void script::parse_func_impl(const iter_t &iter){
		assert(iter->value.id() == freefoil_grammar::func_impl_ID);
		const function_shared_ptr_t parsed_func = parse_func_head(iter->children.begin());

		//is it an implementation of previously declared function?
		const function_shared_ptr_list_t::const_iterator old_func_iter
			 =	std::find_if(
					funcs_list_.begin(),
					funcs_list_.end(),
					boost::bind(&function_heads_equal_functor, _1, parsed_func)
				);
		if (old_func_iter != funcs_list_.end()){
			//found previous declaration

			//if that old function already has implementation, spit an error
			const function_shared_ptr_t old_func = *old_func_iter;
			if (old_func->is_implemented()){
				throw freefoil_exception("function " + old_func->get_name() + " already implemented");
			}
			//if that old function differs from our new one only by some "refs", spit an error
			assert(parsed_func->get_params().size() == old_func->get_params().size());
			if (!std::equal(
							old_func->get_params().begin(), old_func->get_params().end(),
							parsed_func->get_params().begin(),
							&params_refs_equal_functor))
			{
				throw freefoil_exception("previous declaration differs from this one only by \"ref(s)\"");
			}
		}

		//TODO:
		//parsed_func->set_body(parse_func_body(iter->children.begin()+1));
		funcs_list_.push_back(parsed_func);
	}

	param_shared_ptr_t script::parse_func_param(const iter_t &iter){
		assert(iter->value.id() == freefoil_grammar::param_ID);

		value::E_VALUE_TYPE val_type;
		bool is_ref = false;
		std::string val_name = "";

		const std::string val_type_as_str(parse_str(iter->children.begin()));

		if (val_type_as_str == "int"){
			val_type = value::intType;
		}else if (val_type_as_str == "float"){
			val_type = value::floatType;
		}else if (val_type_as_str == "bool"){
			val_type = value::boolType;
		}else{
			assert(val_type_as_str == "string");
			val_type = value::stringType;
		}

		iter_t cur_iter = iter->children.begin() + 1;
		const iter_t iter_end = iter->children.end();
		if (cur_iter != iter_end){
			if (cur_iter->value.id() == freefoil_grammar::ref_ID){
				is_ref = true;
				++cur_iter;
			}
			if (cur_iter != iter_end){
				if (cur_iter->value.id() == freefoil_grammar::ident_ID){
					val_name = parse_str(cur_iter);
					++cur_iter;
				}
			}
		}
		assert(cur_iter == iter_end);
		return param_shared_ptr_t(new param(val_type, is_ref, val_name));
	}

	params_shared_ptr_list_t script::parse_func_params_list(const iter_t &iter){
		assert(iter->value.id() == freefoil_grammar::params_list_ID);
		params_shared_ptr_list_t params_list;
		for (iter_t cur_iter = iter->children.begin(), iter_end = iter->children.end(); cur_iter != iter_end; ++cur_iter){
			 params_list.push_back(parse_func_param(cur_iter));
		}
		return params_list;
	}

	function_shared_ptr_t script::parse_func_head(const iter_t &iter){

		assert(iter->value.id() == freefoil_grammar::func_head_ID);
		assert(iter->children.size() == 3);
		assert((iter->children.begin())->value.id() == freefoil_grammar::func_type_ID);
		function::E_FUNCTION_TYPE func_type;

		const std::string func_type_as_str(parse_str(iter->children.begin()));
		if (func_type_as_str == "void"){
			func_type = function::voidType;
		}else if (func_type_as_str == "string"){
			func_type = function::stringType;
		}else if (func_type_as_str == "int"){
			func_type = function::intType;
		}else if (func_type_as_str == "float"){
			func_type = function::floatType;
		}else{
			assert(func_type_as_str == "bool");
			func_type = function::floatType;
		}
		std::cout << "func_type: " << func_type;

		const function_shared_ptr_t parsed_func = function_shared_ptr_t(new function(parse_str(iter->children.begin()+1), func_type, parse_func_params_list(iter->children.begin()+2)));

		if (std::find_if(
			core_funcs_list_.begin(),
			core_funcs_list_.end(),
			boost::bind(&function_heads_equal_functor, _1, parsed_func)) != core_funcs_list_.end()){
			throw freefoil_exception("unable to override core function");
		}

		return parsed_func;
	}

	void script::parse_func_body(const iter_t &iter){
		assert(iter->value.id() == freefoil_grammar::func_body_ID);
		//TODO:
	}

	std::string script::parse_str(const iter_t &iter){
		return std::string(iter->value.begin(), iter->value.end());
	}

}
