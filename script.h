#ifndef SCRIPT_H_
#define SCRIPT_H_
#define BOOST_SPIRIT_DUMP_PARSETREE_AS_XML 1

#include "freefoil_grammar.h"
#include <iostream>
#include <string>

#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <map>
#endif

namespace Freefoil {
	using namespace Private;
	class script{
		private:
		typedef std::string::const_iterator iterator_t;
		typedef tree_match<iterator_t> parse_tree_match_t;
		typedef parse_tree_match_t::tree_iterator iter_t;

			void process(){
				//TODO:
			}
		public:
			void exec(){
				freefoil_grammar grammar;
				std::string str;
				while (getline(std::cin, str)){
					if (str == "q"){
						break;
					}
					
					std::string::const_iterator iter_begin = str.begin();
					std::string::const_iterator iter_end = str.end();
					
					tree_parse_info<iterator_t> tree_parse_info = ast_parse(iter_begin, iter_end, grammar, space_p);
					if (tree_parse_info.full){
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
            			tree_parse_info.trees, 
            			"", 
            			rule_names);
#endif		
						process();
					}else{
						std::cout << "failed" << std::endl;
						std::cout << "stopped at: " << *(tree_parse_info.stop) << std::endl;	
					}	
				}	
			}
	};
}

#endif /*SCRIPT_H_*/
