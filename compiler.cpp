#include "compiler.h"
#include "freefoil_grammar.h"
#include "AST_defs.h"
#include "tree_analyzer.h"
#include "codegen.h"
#include "freefoil_vm.h"
#include "runtime.h"

#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <map>
#endif

namespace Freefoil {

    using namespace Private;

    static tree_parse_info_t build_AST(const iterator_t &iter_begin, const iterator_t &iter_end) {
        return ast_parse<factory_t>(iter_begin, iter_end, freefoil_grammar(), space_p);
    }

#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
    void compiler::dump_tree(const tree_parse_info_t &info) const {
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
        rule_names[freefoil_grammar::return_stmt_ID] = "return_stmt";
        rule_names[freefoil_grammar::bool_expr_in_parenthesis_ID] = "bool_expr_in_parenthesis";
        rule_names[freefoil_grammar::if_stmt_ID] = "if_stmt";
        rule_names[freefoil_grammar::if_branch_ID] = "if_branch";
        rule_names[freefoil_grammar::elsif_branch_ID] = "elsif_branch";
        rule_names[freefoil_grammar::else_branch_ID] = "else_branch";

        tree_to_xml(std::cerr,
                    info.trees,
                    "",
                    rule_names);
    }
#endif

    Runtime::program_entry_shared_ptr compiler::exec(const string &source, bool optimize, bool show) {

        if (parse(source, the_parse_info)) {
            if (the_tree_analyzer.parse(the_parse_info.trees.begin())) {
                const Runtime::constants_pool &the_constants_pool = the_tree_analyzer.get_parsed_constants_pool();
                return the_codegen.exec(the_parse_info.trees.begin(), the_tree_analyzer.get_parsed_funcs_list(), the_constants_pool, optimize, show);
            }
        }
        return Runtime::program_entry_shared_ptr();
    }

    bool compiler::parse(const std::string &program_source, tree_parse_info_t &result_info) {

        bool is_success;

        std::cout << "parsing begin" << std::endl;

        try {
            result_info = build_AST(iterator_t(program_source.begin(), program_source.end()), iterator_t());
#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
            dump_tree(result_info);
#endif
            is_success = true;
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

            is_success = false;
        }

        std::cout << "parsing end" << std::endl;

        return is_success;
    }

    compiler::compiler(){}
}
