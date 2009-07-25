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
	
	bool params_equal_functor(const param_shared_ptr_t &param, const param_shared_ptr_t &the_param){
		return 		param->get_name() == the_param->get_name() 
				&& 	param->get_value_type() == the_param->get_value_type();
	}
	
	bool function_heads_equal_functor(const function_shared_ptr_t &func, const function_shared_ptr_t &the_func){
		return 		func->get_type() == the_func->get_type() 
				&&  func->get_name() == the_func->get_name()
				&&  func->get_params().size() == the_func->get_params().size()
				&&  std::equal(
								func->get_params().begin(), func->get_params().end(),
								the_func->get_params().begin(), 
								&params_equal_functor); 
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
		//TODO:
		try{
		std::cout << "parsing begin" << std::endl;
		const parser_id id = iter->value.id();
		std::cout << "id = " << id.to_long();
		assert(id == freefoil_grammar::script_ID || id == freefoil_grammar::func_decl_ID || id == freefoil_grammar::func_impl_ID);
		switch (id.to_long()){
			case freefoil_grammar::script_ID:
				parse_script(iter->children.begin());
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
		//TODO:
	}
	
	void script::parse_func_decl(const iter_t &iter){
		//TODO:
	}
	
	void script::parse_func_impl(const iter_t &iter){
		assert(iter->value.id() == freefoil_grammar::func_impl_ID);
		function_shared_ptr_t parsed_function_head = parse_func_head(iter->children.begin());
		//TODO: save parsed_function_head somewhere
		parse_func_body(iter->children.begin()+1);
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
		//TODO:
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
		/*
		//debug only
		for (function_shared_ptr_list_t::const_iterator core_iter = core_funcs_list_.begin(), core_iter_end = core_funcs_list_.end(); core_iter != core_iter_end; ++core_iter){
			const function_shared_ptr_t core_func = *core_iter;
			
			std::cout << std::endl << core_func->get_name() << " " << core_func->get_type() << " " << core_func->get_params().size() << std::endl;
			std::cout << parsed_func->get_name() << " " << parsed_func->get_type() << " " << parsed_func->get_params().size() << std::endl;
			
			if (core_func->get_name() == parsed_func->get_name() && core_func->get_type() == parsed_func->get_type() && core_func->get_params().size() == parsed_func->get_params().size()){
				std::cout << "hehe";
				bool result;
				if (core_func->get_params().empty()){
					result = true;
				}else{
					result = false;
					params_shared_ptr_list_t::const_iterator parsed_func_param_iter = parsed_func->get_params().begin();
					params_shared_ptr_list_t::const_iterator core_func_param_iter  = core_func->get_params().begin();
					params_shared_ptr_list_t::const_iterator core_func_param_end_iter  = core_func->get_params().end();
					std::cout << *core_func_param_iter << " " << *core_func_param_end_iter << std::endl;
					while (core_func_param_iter != core_func_param_end_iter){
						
						std::cout << (*parsed_func_param_iter)->get_name() << (*parsed_func_param_iter)->get_value_type() << std::endl;
						std::cout << (*core_func_param_iter)->get_name() << (*core_func_param_iter)->get_value_type() << std::endl; 
						
						if ((*parsed_func_param_iter)->get_name() == (*core_func_param_iter)->get_name() && (*parsed_func_param_iter)->get_value_type() == (*core_func_param_iter)->get_value_type()){
							result = true;
						}else{
							result  = false;
							break;
						}
						
						++core_func_param_iter;
						++parsed_func_param_iter;	
					}
				}
				if (result){
					std::cout << "error";
					throw freefoil_exception("unable to override core function");
				}
			}	
		}
		//*/
			
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
