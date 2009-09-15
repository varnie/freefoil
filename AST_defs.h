#ifndef FREEFOIL_DEFS_H_
#define FREEFOIL_DEFS_H_

#include <string>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>

namespace Freefoil {
    namespace Private {

        using std::string;
        using BOOST_SPIRIT_CLASSIC_NS::tree_match;
        using BOOST_SPIRIT_CLASSIC_NS::node_val_data_factory;
using BOOST_SPIRIT_CLASSIC_NS::node_iter_data_factory;
        using BOOST_SPIRIT_CLASSIC_NS::tree_parse_info;
        using BOOST_SPIRIT_CLASSIC_NS::ast_parse;
        using BOOST_SPIRIT_CLASSIC_NS::position_iterator2;

//      typedef node_val_data_factory<> factory_t;
//typedef std::string::const_iterator iterator_t;
        typedef position_iterator2<std::string::const_iterator> iterator_t;
        typedef node_iter_data_factory<iterator_t> factory_t;
        typedef tree_match<iterator_t, factory_t> tree_match_t;
        typedef tree_match_t::tree_iterator iter_t;
        typedef tree_match_t::node_t node_t;
        typedef tree_parse_info<iterator_t,factory_t> tree_parse_info_t;
    }
}

#endif /*FREEFOIL_DEFS_H_*/
