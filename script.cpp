#include "script.h"
#include "freefoil_grammar.h"
#include "AST_defs.h"
#include "tree_analyzer.h"

#include <iostream>
#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <map>
#endif

namespace Freefoil {

    using namespace Private;

    static tree_parse_info_t build_AST(const iterator_t &iter_begin, const iterator_t &iter_end){
        return ast_parse<factory_t>(iter_begin, iter_end, freefoil_grammar(), space_p);
    }

    void script::dump_tree(const tree_parse_info_t &info) const{
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
                rule_names[freefoil_grammar::plus_minus_op_ID] = "plus_minus_op";
                rule_names[freefoil_grammar::or_xor_op_ID] = "or_xor_op";
                rule_names[freefoil_grammar::mult_divide_op_ID] = "mult_divide_op";
                rule_names[freefoil_grammar::bool_constant_ID] = "bool_constant";
                rule_names[freefoil_grammar::unary_plus_minus_op_ID] = "unary_plus_minus_op";

                tree_to_xml(std::cerr,
                            info.trees,
                            "",
                            rule_names);
#endif
    }

    void script::exec() {

        std::string str;
        while (getline(std::cin, str)) {
            if (str == "q") {
                break;
            }

            try {
                std::cout << "parsing begin" << std::endl;
                tree_parse_info_t info = build_AST(iterator_t(str.begin(), str.end()), iterator_t());
                std::cout << "parsing end" << std::endl;

                dump_tree(info);

                tree_analyzer the_tree_analyzer;
                if (the_tree_analyzer.parse(info.trees.begin())){
                    //TODO:
                }
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

    script::script(){
    }
    /*possible implicit casts:

                      bool -> string
                      bool -> int

                      string -> int
                      string -> bool
                      string -> float
    */
}
