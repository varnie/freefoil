#include "script.h"
#include "freefoil_grammar.h"
#include <iostream>

#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <map>
#endif

namespace Freefoil{

	using namespace Private;
	void script::exec(){
		
			freefoil_grammar grammar;
			std::string str;
			while (getline(std::cin, str)){
				if (str == "q"){
					break;
				}
				
				iterator_t iter_begin = str.begin();
				iterator_t iter_end = str.end();
				
				tree_parse_info<iterator_t> info = ast_parse(iter_begin, iter_end, grammar, space_p);
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
	
	void script::parse(const Private::iter_t &iter){	
		//TODO:
		std::cout << "parsing begin" << std::endl;
		parser_id id = iter->value.id();
		std::cout << "id = " << id.to_long();
		std::cout << "parsing end" << std::endl;
	}
	
	script::script(){
	}
}
