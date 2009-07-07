#ifndef FREEFOIL_DEFS_H_
#define FREEFOIL_DEFS_H_

#include <string>
#include <boost/spirit/tree/ast.hpp>

namespace Freefoil{
	namespace Private{
		
		using std::string;
		using BOOST_SPIRIT_CLASSIC_NS::tree_match;
		
		typedef string::const_iterator iterator_t;
		typedef tree_match<iterator_t> tree_match_t;
		typedef tree_match_t::tree_iterator iter_t;		
	}	
}

#endif /*FREEFOIL_DEFS_H_*/
