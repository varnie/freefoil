#ifndef FREEFOIL_GRAMMAR_H_
#define FREEFOIL_GRAMMAR_H_

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_distinct.hpp>
#include <boost/spirit/include/classic_lists.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_utility.hpp>


#define GRAMMAR_RULE(RULE_ID) rule<ScannerT, parser_context<>, parser_tag<RULE_ID> >

namespace Freefoil {
    namespace Private {

        using BOOST_SPIRIT_CLASSIC_NS::symbols;
        using BOOST_SPIRIT_CLASSIC_NS::distinct_parser;
        using BOOST_SPIRIT_CLASSIC_NS::grammar;
        using BOOST_SPIRIT_CLASSIC_NS::rule;
        using BOOST_SPIRIT_CLASSIC_NS::parser_context;
        using BOOST_SPIRIT_CLASSIC_NS::lexeme_d;
        using BOOST_SPIRIT_CLASSIC_NS::token_node_d;
        using BOOST_SPIRIT_CLASSIC_NS::parser_id;
        using BOOST_SPIRIT_CLASSIC_NS::space_p;
        using BOOST_SPIRIT_CLASSIC_NS::list_p;
        using BOOST_SPIRIT_CLASSIC_NS::ch_p;
        using BOOST_SPIRIT_CLASSIC_NS::parser_tag;
        using BOOST_SPIRIT_CLASSIC_NS::no_node_d;
        using BOOST_SPIRIT_CLASSIC_NS::eps_p;
        using BOOST_SPIRIT_CLASSIC_NS::alpha_p;
        using BOOST_SPIRIT_CLASSIC_NS::alnum_p;
        using BOOST_SPIRIT_CLASSIC_NS::discard_node_d;
        using BOOST_SPIRIT_CLASSIC_NS::tree_parse_info;
        using BOOST_SPIRIT_CLASSIC_NS::gen_pt_node_d;
        using BOOST_SPIRIT_CLASSIC_NS::gen_ast_node_d;
        using BOOST_SPIRIT_CLASSIC_NS::inner_node_d;
        using BOOST_SPIRIT_CLASSIC_NS::infix_node_d;
        using BOOST_SPIRIT_CLASSIC_NS::real_p;
        using BOOST_SPIRIT_CLASSIC_NS::uint_p;
        using BOOST_SPIRIT_CLASSIC_NS::str_p;
        using BOOST_SPIRIT_CLASSIC_NS::confix_p;
        using BOOST_SPIRIT_CLASSIC_NS::c_escape_ch_p;
        using BOOST_SPIRIT_CLASSIC_NS::longest_d;
         using BOOST_SPIRIT_CLASSIC_NS::root_node_d;

        struct freefoil_keywords : symbols<int> {
            freefoil_keywords() {
                add
                ("void")
                ("string")
                ("float")
                ("int")
                ("ref")
                ("bool")
                ("true")
                ("false")
                ("+")
                ("-")
                ("*")
                ("/")
                ("==")
                ("!=")
                (">")
                ("<")
                (">=")
                ("<=")
                ("=")
                ("or")
                ("and")
                ("not")

                //TODO: add all the left keywords
                ;
            }
        };
        const freefoil_keywords freefoil_keywords_p;

        const distinct_parser<> keyword_p("a-zA-Z0-9_");

        struct freefoil_grammar : public grammar<freefoil_grammar> {

            enum ruleID {
                script_ID = 0,
                func_decl_ID,
                func_impl_ID,
                ident_ID,
                stmt_end_ID,
                func_head_ID,
                func_body_ID,
                params_list_ID,
                param_ID,
                ref_ID,
                func_type_ID,
                var_type_ID,
                stmt_ID,
                var_declare_stmt_list_ID,
                expr_ID,
                term_ID,
                factor_ID,
                number_ID,
                bool_expr_ID,
                bool_term_ID,
                bool_factor_ID,
                bool_relation_ID,
                quoted_string_ID,
                func_call_ID,
                invoke_args_list_ID,
                block_ID,
                var_declare_tail_ID,
                assign_op_ID,
                cmp_op_ID,
                or_tail_ID,
                and_tail_ID,
                relation_tail_ID,
                plus_minus_op_ID,
                mult_divide_op_ID,
                bool_constant_ID,
            };

            template <typename ScannerT>
            struct definition {
                definition(freefoil_grammar const &/*self*/) {

                    script = *(func_decl | func_impl) >> no_node_d[eps_p];

                    ident = lexeme_d[
                                token_node_d[
                                    (((alpha_p | ch_p('_')) >> *(alnum_p | ch_p('_'))))
                                    -
                                    freefoil_keywords_p
                                ]
                            ];

                    stmt_end = discard_node_d[ch_p(';')];

                    func_decl = func_head >> stmt_end;

                    func_impl = func_head >> gen_pt_node_d[func_body];

                    func_head = func_type >> ident >> gen_pt_node_d[params_list];

                    func_body = gen_ast_node_d[no_node_d[ch_p('{')] >> *stmt >> no_node_d[ch_p('}')]];

                    func_type = keyword_p("string") | keyword_p("void") | keyword_p("float") | keyword_p("int") | keyword_p("bool");

                    params_list = no_node_d[ch_p('(')]
                                  >>!(gen_pt_node_d[param] >> *(no_node_d[ch_p(',')] >> gen_pt_node_d[param]))
                                  >> no_node_d[ch_p(')')];

                    func_call = ident >> gen_pt_node_d[invoke_args_list];

                    invoke_args_list = no_node_d[ch_p('(')]
                                 >> !(gen_pt_node_d[bool_expr] >> *(no_node_d[ch_p(',')] >> gen_pt_node_d[bool_expr]))
                                 >> no_node_d[ch_p(')')];

                    param = var_type >> !ref >> !ident;

                    ref = keyword_p("ref");

                    var_type = keyword_p("string") | keyword_p("float") | keyword_p("int") | keyword_p("bool");

                    param = var_type >> !ref >> !ident;

                    ref = keyword_p("ref");

                    var_declare_stmt_list = var_type >> gen_pt_node_d[var_declare_tail] >> *(no_node_d[ch_p(',')] >> gen_pt_node_d[var_declare_tail]) >> no_node_d[stmt_end];

                    var_declare_tail = ident >> !(assign_op >> gen_pt_node_d[bool_expr]);

                    assign_op = ch_p("=") | str_p("+=") | str_p("-=") | str_p("*=") | str_p("/=");

                    cmp_op = str_p("<=") | str_p(">=") | str_p("==") | str_p("!=") | ch_p(">") | ch_p("<");

                    bool_expr = gen_pt_node_d[bool_term] >> or_tail;

                    or_tail = *(str_p("or") >> gen_pt_node_d[bool_term]);

                    bool_term = gen_pt_node_d[bool_factor] >> and_tail;

                    and_tail = *(str_p("and") >> gen_pt_node_d[bool_factor]);

                    bool_factor = !str_p("not") >> gen_pt_node_d[bool_relation];

                    bool_relation = gen_pt_node_d[expr] >> relation_tail;

                    relation_tail = *(cmp_op >> gen_pt_node_d[expr]);

                    expr = !plus_minus_op >> gen_pt_node_d[term] >> *(plus_minus_op >> gen_pt_node_d[term]);

                    plus_minus_op = lexeme_d[ch_p("+") | ch_p("-")];

                    term = gen_pt_node_d[factor] >> *(mult_divide_op >> gen_pt_node_d[factor]);

                    mult_divide_op = lexeme_d[ch_p("*") | ch_p("/")];

                    bool_constant = keyword_p("true") | keyword_p("false");

                    factor = func_call
                             | ident
                             | number
                             | quoted_string
                             | no_node_d[ch_p('(')] >> bool_expr >> no_node_d[ch_p(')')]
                             | bool_constant
                    ;

                    quoted_string = token_node_d[lexeme_d[confix_p('"', *c_escape_ch_p, '"')]];

                    number = token_node_d[longest_d[uint_p | real_p]];

                    stmt =
                        stmt_end |
                        var_declare_stmt_list |
                        func_call >> no_node_d[stmt_end] |
                        gen_pt_node_d[block]; //TODO: add other alternatives

                    block = gen_ast_node_d[no_node_d[ch_p('{')] >> *stmt >> no_node_d[ch_p('}')]];

                    // example of turning on the debugging info.
                    BOOST_SPIRIT_DEBUG_RULE(script);
                }

                GRAMMAR_RULE(script_ID) const &start() const {
                    return script;
                }

                /////////////////////////////////////////////////////////////////////
                GRAMMAR_RULE(script_ID) script;
                GRAMMAR_RULE(func_decl_ID) func_decl;
                GRAMMAR_RULE(func_impl_ID) func_impl;
                GRAMMAR_RULE(ident_ID) ident;
                GRAMMAR_RULE(stmt_end_ID) stmt_end;
                GRAMMAR_RULE(func_head_ID) func_head;
                GRAMMAR_RULE(func_body_ID) func_body;
                GRAMMAR_RULE(params_list_ID) params_list;
                GRAMMAR_RULE(param_ID) param;
                GRAMMAR_RULE(func_type_ID) func_type;
                GRAMMAR_RULE(var_type_ID) var_type;
                GRAMMAR_RULE(ref_ID) ref;
                GRAMMAR_RULE(stmt_ID) stmt;
                GRAMMAR_RULE(var_declare_stmt_list_ID) var_declare_stmt_list;
                GRAMMAR_RULE(expr_ID) expr;
                GRAMMAR_RULE(term_ID) term;
                GRAMMAR_RULE(factor_ID) factor;
                GRAMMAR_RULE(number_ID) number;
                GRAMMAR_RULE(bool_expr_ID) bool_expr;
                GRAMMAR_RULE(bool_term_ID) bool_term;
                GRAMMAR_RULE(bool_factor_ID) bool_factor;
                GRAMMAR_RULE(bool_relation_ID) bool_relation;
                GRAMMAR_RULE(quoted_string_ID) quoted_string;
                GRAMMAR_RULE(func_call_ID) func_call;
                GRAMMAR_RULE(invoke_args_list_ID) invoke_args_list;
                GRAMMAR_RULE(block_ID) block;
                GRAMMAR_RULE(var_declare_tail_ID) var_declare_tail;
                GRAMMAR_RULE(assign_op_ID) assign_op;
                GRAMMAR_RULE(cmp_op_ID) cmp_op;
                GRAMMAR_RULE(or_tail_ID) or_tail;
                GRAMMAR_RULE(and_tail_ID) and_tail;
                GRAMMAR_RULE(relation_tail_ID) relation_tail;
                GRAMMAR_RULE(plus_minus_op_ID) plus_minus_op;
                GRAMMAR_RULE(mult_divide_op_ID) mult_divide_op;
                GRAMMAR_RULE(bool_constant_ID) bool_constant;
            };
        };
    }
}

#endif /*SYNTAX_H_*/
