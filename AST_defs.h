#ifndef FREEFOIL_DEFS_H_
#define FREEFOIL_DEFS_H_

#include <string>
#include <boost/spirit/include/classic_ast.hpp>

namespace Freefoil {
    namespace Private {

        using std::string;
        using BOOST_SPIRIT_CLASSIC_NS::tree_match;
        using BOOST_SPIRIT_CLASSIC_NS::node_val_data_factory;
        using BOOST_SPIRIT_CLASSIC_NS::tree_parse_info;
        using BOOST_SPIRIT_CLASSIC_NS::ast_parse;

        struct node_info {
            union {
                unsigned char function_index;
                unsigned char stack_offset;
            };
            enum E_CONVERT {
                NONE = -1,
                INT_TO_STR,
                INT_TO_FLOAT,
                INT_TO_BOOL,
                FLOAT_TO_STR,
                BOOL_TO_STR,
                BOOL_TO_INT
            };
            E_CONVERT convert;
            node_info():convert(NONE)
            {}
        };

        typedef node_val_data_factory<node_info> factory_t;
        typedef string::const_iterator iterator_t;
        typedef tree_match<iterator_t, factory_t> tree_match_t;
        typedef tree_match_t::tree_iterator iter_t;
        typedef tree_match_t::node_t node_t;
        typedef tree_parse_info<iterator_t,factory_t> tree_parse_info_t;
    }
}

#endif /*FREEFOIL_DEFS_H_*/
